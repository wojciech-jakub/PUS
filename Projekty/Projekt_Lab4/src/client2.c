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
#include <netinet/sctp.h>
#include <time.h>
#define BUFF_SIZE 256

int main(int argc, char** argv) {

    int                     sockfd;
    int                     retval, bytes, slen, i, flags;
    char                    *retptr;
    struct addrinfo         hints, *result;
    struct sctp_initmsg     initmsg;
    struct sctp_status      s_status;
    struct sctp_sndrcvinfo  s_sndrcvinfo;
    struct sctp_event_subscribe s_events;
    char                    buffer[BUFF_SIZE];

    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IP ADDRESS> <PORT NUMBER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family         = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype       = SOCK_STREAM;
    hints.ai_flags          = 0;
    hints.ai_protocol       = 0;

    retval = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (retval != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }
    if (result == NULL) {
        fprintf(stderr, "Could not connect!\n");
        exit(EXIT_FAILURE);
    }

    sockfd = socket(result->ai_family, result->ai_socktype, IPPROTO_SCTP);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&initmsg, 0, sizeof(initmsg));
    initmsg.sinit_num_ostreams = 4;
    initmsg.sinit_max_instreams = 3;
    initmsg.sinit_max_attempts = 5;

    retval = setsockopt(sockfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof (initmsg));

    if (retval != 0){
        perror("setsockopt()");;
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, result->ai_addr, result->ai_addrlen) == -1) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    memset (&s_events, 0, sizeof (s_events));
    s_events.sctp_data_io_event = 1;
    retval = setsockopt (sockfd, SOL_SCTP, SCTP_EVENTS,(const void *) &s_events, sizeof (s_events));

    slen = sizeof(s_status);
    retval = getsockopt(sockfd, SOL_SCTP, SCTP_STATUS,(void *) &s_status, (socklen_t *) & slen);

    printf ("ID: %d\n", s_status.sstat_assoc_id);
    printf ("Stan: %d\n", s_status.sstat_state);
    printf ("Liczba strumieni wychodzacych: %d\n", s_status.sstat_instrms);
    printf ("Liczba strumieni przychodzacych: %d\n", s_status.sstat_outstrms);

    for (i = 0; i <initmsg.sinit_num_ostreams - 1; i++) {
        memset(buffer, 0, sizeof(buffer));
        retval = sctp_recvmsg(sockfd, (void *) buffer, BUFF_SIZE,(struct sockaddr *) NULL, 0, &s_sndrcvinfo, &flags);

        if (retval > 0){
            buffer[retval] = 0;
            printf ("Nr strumienia: %d %s\n", s_sndrcvinfo.sinfo_stream, buffer);

        }
    }
    close(sockfd);
    exit(EXIT_SUCCESS);
}
