/*
 * Data:                2009-03-15
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc ioarp.c -o ioarp
 * Uruchamianie:        $ ./ioarp <adres IPv4> <adres MAC>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h> /* inet_pton() */
#include <net/if_arp.h>
#include <netinet/in.h> /* struct sockaddr_in */
#include <sys/ioctl.h>

int main(int argc, char** argv) {

    int                     sockfd, retval;
    struct arpreq           request;

    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IPv4 ADDRESS> <MAC ADDRESS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&request, 0, sizeof(struct arpreq));

    ((struct sockaddr_in*)&(request.arp_pa))->sin_family = AF_INET;
    inet_pton(
        AF_INET, argv[1],
        &(((struct sockaddr_in*) &(request.arp_pa))->sin_addr.s_addr)
    );

    /* Adres MAC: */
    request.arp_ha.sa_family = ARPHRD_ETHER;

    retval = sscanf(
                 argv[2], "%2x:%2x:%2x:%2x:%2x:%2x",
                 (unsigned int*)&request.arp_ha.sa_data[0],
                 (unsigned int*)&request.arp_ha.sa_data[1],
                 (unsigned int*)&request.arp_ha.sa_data[2],
                 (unsigned int*)&request.arp_ha.sa_data[3],
                 (unsigned int*)&request.arp_ha.sa_data[4],
                 (unsigned int*)&request.arp_ha.sa_data[5]
             );

    if (retval != 6) {
        fprintf(stderr, "Invalid address format!\n");
        exit(EXIT_FAILURE);
    }

    request.arp_flags = ATF_COM; /* Wymagana flaga - completed ARP entry. */
    request.arp_flags |= ATF_PERM; /* Wpis permanentny. */

    retval = ioctl(sockfd, SIOCSARP, &request);
    if (retval == -1) {
        perror("ioctl()");
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    exit(EXIT_SUCCESS);
}
