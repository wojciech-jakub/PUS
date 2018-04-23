#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <time.h>
#define BUFF_SIZE 256

int main(int argc, char** argv) {

    int                     listenfd, connfd;
    int                     retval, bytes;
    struct sockaddr_in      servaddr;
    struct sctp_initmsg     initmsg;
    char                    buffer[BUFF_SIZE];

    // do pobrania czasu czasu
    time_t                  t;
    struct tm               tm;

    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT NUMBER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (listenfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family             =       AF_INET;
    servaddr.sin_port               =       htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr        =       htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    // definiowanie strumieni
    memset (&initmsg, 0, sizeof (initmsg));
    initmsg.sinit_num_ostreams = 3;
    initmsg.sinit_max_instreams = 50;

    retval = setsockopt (listenfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof (initmsg));
    if (retval != 0) {
        perror("setsockopt()");;
        exit(EXIT_FAILURE);
    }
    if (listen(listenfd, 5) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    while(1) {
        connfd = accept(listenfd, NULL, (int *) NULL);

        if (connfd == -1)
	    {
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        printf ("server accept\n");
        fflush(stdout);

        // pobranie daty i czasu
        t = time(NULL);
        tm = *localtime(&t);
        memset(buffer, 0, sizeof(buffer));
        sprintf (buffer, "Aktualna data: %d.%d.%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);

	    // wyslanie daty
        retval = sctp_sendmsg (connfd, buffer, (size_t) strlen (buffer), NULL, 0, 0, 0, 0, 0, 0);

        if (retval == -1)
        {
            perror("sctp_sendmsg() data");
            exit(EXIT_FAILURE);
        }

        memset(buffer, 0, sizeof(buffer));
        sprintf (buffer, "Aktualny czas: %d:%d:%d", tm.tm_hour, tm.tm_min, tm.tm_sec);

        //wyslanie czasu
        retval = sctp_sendmsg (connfd, buffer, (size_t) strlen (buffer), NULL, 0, 0, 0, 1, 0, 0);

        if (retval == -1)
        {
            perror("sctp_sendmsg() czas");
            exit(EXIT_FAILURE);
        }
    }
    close(listenfd);
    exit(EXIT_SUCCESS);
}
