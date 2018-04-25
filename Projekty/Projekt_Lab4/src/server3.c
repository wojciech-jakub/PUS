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
    int                     retval, bytes, flags, slen;
    struct sockaddr_in      servaddr, cliaddr;
    struct sctp_initmsg     initmsg;
    struct sctp_sndrcvinfo  s_sndrcvinfo;
    struct sctp_status      s_status;
    char                    buffer[BUFF_SIZE];
    struct sctp_event_subscribe s_events;
    socklen_t               from_len;
    int                     tryb, stream;

    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <PORT NUMBER> <tryb 0 lub 1>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    tryb = atoi(argv[2]);
    
    if(tryb != 0 && tryb != 1){
        fprintf(stderr, "Tryb 0 lub 1");
        exit(EXIT_FAILURE);
    }

    listenfd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
    if (listenfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_port        = htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    memset (&initmsg, 0, sizeof (initmsg));
    initmsg.sinit_num_ostreams = 5;
    initmsg.sinit_max_instreams = 5;

    retval = setsockopt (listenfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof (initmsg));
    if (retval != 0) {
        perror("setsockopt()");;
        exit(EXIT_FAILURE);
    }
    if (listen(listenfd, 5) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }


    memset (&s_events, 0, sizeof (s_events));
    s_events.sctp_data_io_event = 1;
    retval = setsockopt (listenfd, SOL_SCTP, SCTP_EVENTS,(const void *) &s_events, sizeof (s_events));

    slen = sizeof(s_status);
    retval = getsockopt(listenfd, SOL_SCTP, SCTP_STATUS,(void *) &s_status, (socklen_t *) & slen);


    while(1){
        flags = 0;
        from_len = (socklen_t)sizeof(struct sockaddr_in);
        memset (&s_sndrcvinfo, 0, sizeof (s_sndrcvinfo));
        memset(buffer, 0, sizeof(buffer));
        retval = sctp_recvmsg(listenfd,buffer, sizeof(buffer),(struct sockaddr *)&cliaddr, &from_len, &s_sndrcvinfo, &flags);
        printf("Wiadomosc: %s", buffer);

        stream = s_sndrcvinfo.sinfo_stream;

        if(tryb == 0){ /* zawsze strumien 0 */
            printf("Strumien: %d\n", stream);
            retval = sctp_sendmsg (listenfd, buffer, (size_t) strlen (buffer),(struct sockaddr *)&cliaddr, from_len, 0, 0, stream, 0, 0);
        }else if(tryb == 1){ /* strumien o 1 wiekszy */
            if((stream + 1) < initmsg.sinit_num_ostreams)
                stream++;
            else
                stream = 0;

            printf("Strumien: %d\n", stream);
            fflush(stdout);
            retval = sctp_sendmsg (listenfd, buffer, (size_t) strlen (buffer),(struct sockaddr *)&cliaddr, from_len, 0, 0, stream, 0, 0);
        }

        if (retval == -1) {
            perror("sctp_sendmsg()");
            exit(EXIT_FAILURE);
        }
    }

    close(listenfd);
    exit(EXIT_SUCCESS);
}