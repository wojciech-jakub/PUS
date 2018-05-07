#define _GNU_SOURCE     /* getresgid() */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>


int main(int argc, char** argv) {

    int             listenfd, connfd;
    struct          sockaddr_in client_addr, server_addr;
    socklen_t       client_addr_len, server_addr_len;
    char            addr_buff[256];
    unsigned int    port_number;
    int             user_id;

    uid_t           ruid, euid, suid; /* Identyfikatory uzytkownika. */

    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <PORT> <UID>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    port_number = atoi(argv[1]);
    user_id = atoi(argv[2]);

    if (port_number < 1024) {
        if (geteuid() != 0) {
            fprintf(stderr, "You need root privileges to "
                    "bind to well-known port (<1024).\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Utworzenie gniazda dla protokolu TCP: */
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }


    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family          =       AF_INET;
    server_addr.sin_addr.s_addr     =       htonl(INADDR_ANY);
    server_addr.sin_port            =       htons(port_number);
    server_addr_len                 =       sizeof(server_addr);


    /* Pobranie identyfikatorow uzytkownika (real, effective, save set). */
    if (getresuid(&ruid, &euid, &suid) == -1) {
        perror("getresgid()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "UID: %u, EUID: %u, SUID: %u\n", ruid, euid, suid);


    fprintf(stdout, "Binding to port %u...\n", port_number);
    /* Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem: */
    if (bind(listenfd, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }


    // set UID
    setuid(user_id);

    if (getresuid(&ruid, &euid, &suid) == -1) {
        perror("getresgid()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "UID: %u, EUID: %u, SUID: %u\n", ruid, euid, suid);

    /* Program powinien wykonywac sie dalej z EUID != 0 i saved set-user-ID != 0. */
    if (euid == 0 || suid == 0) {
        fprintf(stderr, "Run server as unprivileged user!\n");
        exit(EXIT_FAILURE);
    }


    /* Przeksztalcenie gniazda w gniazdo nasluchujace: */
    if (listen(listenfd, 2) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server is listening for incoming connection...\n");

    /* Funkcja pobiera polaczenie z kolejki polaczen oczekujacych na zaakceptowanie
     * i zwraca deskryptor dla gniazda polaczonego: */
    client_addr_len = sizeof(client_addr);
    connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (connfd == -1) {
        perror("accept()");
        exit(EXIT_FAILURE);
    }

    fprintf(
        stdout, "TCP connection accepted from %s:%d\n",
        inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
        ntohs(client_addr.sin_port)
    );

    sleep(5);

    fprintf(stdout, "Closing connected socket (sending FIN to client)...\n");
    close(connfd);

    close(listenfd);

    exit(EXIT_SUCCESS);
}
