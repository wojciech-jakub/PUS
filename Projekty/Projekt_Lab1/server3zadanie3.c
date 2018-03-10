/*
 * Data:                2009-02-10
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc server2.c -o server2
 * Uruchamianie:        $ ./server2 <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include "libpalindrome.h"

int main(int argc, char** argv) {

    int             sockfd; /* Deskryptor gniazda. */
    int             retval; /* Wartosc zwracana przez funkcje. */

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct          sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t       client_addr_len, server_addr_len;

    /* Bufor wykorzystywany przez recvfrom() i sendto(): */
    char            buff[256];

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char            addr_buff[256];

    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu UDP: */
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family          =       AF_INET;
    server_addr.sin_addr.s_addr     =       htonl(INADDR_ANY);
    server_addr.sin_port            =       htons(atoi(argv[1]));
    server_addr_len                 =       sizeof(server_addr);

    /* Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem: */
    if (bind(sockfd, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server is listening for incoming connection...\n");

    while(1){
        client_addr_len = sizeof(client_addr);
        /* Oczekiwanie na dane od klienta: */
        retval = recvfrom(
                    sockfd,
                    buff, sizeof(buff),
                    0,
                    (struct sockaddr*)&client_addr, &client_addr_len
                );
        if (retval == -1) {
            perror("recvfrom()");
            exit(EXIT_FAILURE);
        }

        if(retval == 0 || buff[0] == '\0'){
            break;
        }

        fprintf(stdout, "UDP datagram received from %s:%d.\n",
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
            ntohs(client_addr.sin_port)
        );

        char palindrome[256];
        switch(is_palindrome(buff, strlen(buff))){
            case -1:
            //dane w buforze zawieraja znaki, ktore nie sa cyframi lub znakami bialymi
                sprintf( palindrome, "Nie jest liczba" );
                break;
            case 0:
            // dane w buforze nie sa palindromem liczbowym
                sprintf( palindrome, "Nie palindrom" );
                break;
            case 1:
            //dane w buforze sa palindromem liczbowym*
                sprintf( palindrome, "Palindrom" );
                break;

        }

        retval = sendto(
                    sockfd,
                    palindrome, sizeof(palindrome),
                    0,
                    (struct sockaddr*)&client_addr, client_addr_len
                );
        if (retval == -1) {
            perror("sendto()");
            exit(EXIT_FAILURE);
        }

        memset(&buff, 0, sizeof(buff));
    }



    close(sockfd);

    exit(EXIT_SUCCESS);
}
