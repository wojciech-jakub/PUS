#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef __USE_BSD
#define __USE_BSD
#endif
#include <netinet/ip.h>
#define __FAVOR_BSD
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
// #include <error.h>
#include "checksum.h"

#define SOURCE_PORT 5555
#define SOURCE_ADDRESS "192.168.0.250"

struct phdr {
    struct in_addr ip_src, ip_dst;
    unsigned char unused;
    unsigned char protocol;
    unsigned short length;

};

int main(int argc, char** argv) {

    int     sockfd;
    int     offset_option;
    int     retval;

    struct          addrinfo        hints;
    struct          addrinfo        *rp, *result;

    unsigned short  checksum;
    unsigned char   datagram[sizeof(struct udphdr) + sizeof(struct phdr)] = {0};

    struct udphdr   *udp_header     = (struct udphdr *)(datagram);
    struct phdr     *pseudo_header  = (struct phdr *) (datagram + sizeof(struct udphdr));


    if (argc != 3) {
        fprintf(stderr,"Invocation: %s <HOSTNAME OR IPv6 ADDRESS> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family         =       AF_INET6;
    hints.ai_socktype       =       SOCK_RAW;
    hints.ai_protocol       =       IPPROTO_UDP;


    retval = getaddrinfo(argv[1], NULL, &hints, &result);
    if (retval != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(retval));
        exit(EXIT_FAILURE);
    }


    //

    offset_option = 6;

    for (rp = result; rp != NULL; rp = rp->ai_next) {

        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            perror("socket()");
            continue;
        }

        /* Ustawienie opcji IP_HDRINCL: */
        retval = setsockopt(
                     sockfd,
                     IPPROTO_IPV6, IPV6_CHECKSUM,
                     &offset_option, sizeof(int)
                 );

        if (retval == -1) {
            perror("setsockopt()");
            exit(EXIT_FAILURE);
        } else {
            printf("created\n");
            break;
        }
    }

    if (rp == NULL) {
        fprintf(stderr, "Client failure: could not create socket.\n");
        exit(EXIT_FAILURE);
    }

    //
    /*********************************/
    /* Wypelnienie pol naglowka UDP: */
    /*********************************/



    udp_header->uh_dport            =       htons(atoi(argv[2]));
    udp_header->uh_ulen             =       htons(sizeof(struct udphdr));


    pseudo_header->ip_dst.s_addr     =       ((struct sockaddr_in*)rp->ai_addr)->sin_addr.s_addr;
    pseudo_header->unused           =       0;
    pseudo_header->protocol         =       IPPROTO_UDP;
    pseudo_header->length           =       udp_header->uh_ulen;
    udp_header->uh_sum              =       0;

    checksum                        =       internet_checksum(
                                                (unsigned short *)udp_header,
                                                sizeof(struct udphdr)
                                                + sizeof(struct phdr)
                                            );

    udp_header->uh_sum              = (checksum == 0) ? 0xffff : checksum;


    fprintf(stdout, "Sending UDP...\n");

    for (;;) {

        retval = sendto(
                     sockfd,
                     datagram, sizeof(struct udphdr),
                     0,
                     rp->ai_addr, rp->ai_addrlen
                 );

        if (retval == -1) {
            perror("sendto()");
        }else{
            printf("Send UDP\n");
        }

        sleep(1);
    }

    exit(EXIT_SUCCESS);
}
