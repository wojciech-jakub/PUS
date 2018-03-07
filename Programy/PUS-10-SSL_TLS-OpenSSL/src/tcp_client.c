/*
 * Data:                2009-05-25
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc tcp_client.c -o tcp_client
 * Uruchamianie:        $ ./tcp_client <adres IP> <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>

#define BUF_SIZE 1024

int main(int argc, char** argv) {

    int             sockfd;                 /* Desktryptor gniazda. */
    int             retval;                 /* Wartosc zwracana przez funkcje. */
    struct          sockaddr_in remote_addr;/* Gniazdowa struktura adresowa. */
    socklen_t       addr_len;               /* Rozmiar struktury w bajtach. */
    char            buff[BUF_SIZE];         /* Bufor dla funkcji read(). */
    int             bytes;                  /* Liczba bajtow. */
    int             total;

    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IPv4 ADDRESS> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu TCP: */
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej dla adresu zdalnego (serwera): */
    memset(&remote_addr, 0, sizeof(remote_addr));

    /* Domena komunikacyjna (rodzina protokolow): */
    remote_addr.sin_family = AF_INET;

    /* Konwersja adresu IP z postaci czytelnej dla czlowieka: */
    retval = inet_pton(AF_INET, argv[1], &remote_addr.sin_addr);
    if (retval == 0) {
        fprintf(stderr, "inet_pton(): invalid network address!\n");
        exit(EXIT_FAILURE);
    } else if (retval == -1) {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }

    remote_addr.sin_port = htons(atoi(argv[2])); /* Numer portu. */
    addr_len = sizeof(remote_addr); /* Rozmiar struktury adresowej w bajtach. */

    /* Nawiazanie polaczenia (utworzenie asocjacji,
     * skojarzenie adresu zdalnego z gniazdem): */
    if (connect(sockfd, (const struct sockaddr*) &remote_addr, addr_len) == -1) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }


    /* Odebranie danych: */
    bytes = 0;
    total = 0;
    while ((bytes = read(sockfd, buff + total, BUF_SIZE - total)) > 0) {
        total += bytes;
    }

    if (bytes == -1) {
        perror("read()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Received message: ");
    fwrite(buff, sizeof(char), total, stdout);
    fprintf(stdout, "\n");

    close(sockfd);
    exit(EXIT_SUCCESS);
}
