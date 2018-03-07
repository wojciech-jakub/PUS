/*
 * Data:                2009-03-15
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc iolist.c -o iolist
 * Uruchamianie:        $ ./iolist
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <net/if.h> /* struct ifconf, struct ifreq */
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, char** argv) {

    int                     sockfd; /* Deskryptor gniazda */
    int                     retval;
    int                     len, prev_len;
    char                    *buff, *ptr;
    char                    address[INET_ADDRSTRLEN];
    struct ifconf           ifc;
    struct ifreq            *ifreqptr, ifreqstruct;

    if (argc != 1) {
        fprintf(stderr, "Invocation: %s\n", argv[0]);
    }

    /* Deskryptor gniazda bedzie potrzebny w wywolaniu funkcji ioctl(). */
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    prev_len        =       0;
    /* Poczatkowy rozmiar bufora dla ioctl(): */
    len             =       20 * sizeof(struct ifreq);

    /*
     * Petla wykonuje sie dopoki rozmiar zwroconej tablicy struktur
     * miedzy dwoma kolejnymi wywolaniami nie ulegnie zmiane.
     * Zakonczenie petli bedzie oznaczac ze tablica nie zostala przycieta.
     */
    for (;;) {

        buff = (char*)malloc(sizeof(char) * len);
        if (buff == NULL) {
            fprintf(stderr, "malloc() failed!\n");
            break;
        }

        ifc.ifc_len     =       len;
        ifc.ifc_buf     =       buff;

        /* Pobranie informacji konfiguracyjnych na temat interfejsow: */
        retval = ioctl(sockfd, SIOCGIFCONF, &ifc);

        if (retval == -1) {
            /* W przypadku bledu: */
            perror("ioctl()");
            exit(EXIT_FAILURE);
        } else {
            if (ifc.ifc_len == prev_len) {
                /*
                 * Za 2 razem rozmiar nie ulegl zmianie.
                 * Jestesmy pewni, ze bufor jest wystarczajaco duzy i
                 * rezultat wywolania zmiesci sie w buforze
                 * (tablica nie zostanie przycieta).
                 */
                break;
            }
            /* Zapamietanie poprzedniego rozmiaru bufora. */
            prev_len = ifc.ifc_len;
        }

        /* Zwalniamy pamiec. Kolejny bufor bedzie mial wiekszy rozmiar. */
        free(buff);
        /* Zwiekszamy rozmiar bufora (co najmniej raz). */
        len += 10 * sizeof(struct ifreq);
    }

    for (ptr = buff; ptr < buff + ifc.ifc_len; ptr += sizeof(struct ifreq)) {
        ifreqptr = (struct ifreq *)ptr;

        /*
         *  SIOCGIFCONF
         *  Return  a  list  of interface (transport layer) addresses.  This
         *  currently means only addresses of the AF_INET (IPv4) family  for
         *  compatibility.   The  user passes a ifconf structure as argument
         *  to the ioctl.  It contains a pointer to an array of ifreq struc‐
         *  tures in ifc_req and its length in bytes in ifc_len.  The kernel
         *  fills the ifreqs with all current L3  interface  addresses  that
         *  are running: ifr_name contains the interface name (eth0:1 etc.),
         *  ifr_addr the address.
         */

        /*  Tylko IPv4: */
        if (ifreqptr->ifr_addr.sa_family != AF_INET) {
            continue;
        }

        /* Nazwa interfejsu: */
        fprintf(stdout, "Interface: %s ", ifreqptr->ifr_name);

        /* Kopiowanie struktury.
         * W 'ifreqstruct' wazna jest nazwa interfejsu. Dla interfejsu
         * o tej nazwie pobrane zostana flagi, a pozniej maska: */
        ifreqstruct = *ifreqptr;
        /* Pobranie flag: */
        retval = ioctl(sockfd, SIOCGIFFLAGS, &ifreqstruct);
        if (retval == -1) {
            perror("ioctl()");
            exit(EXIT_FAILURE);
        }

        fprintf(
            stdout,
            "<%s %s>",
            (ifreqstruct.ifr_flags & IFF_UP) ? "UP" : "DOWN",
            (ifreqstruct.ifr_flags & IFF_PROMISC) ? "PROMISC" : "NO-PROMISC"
        );

        /* Konwersja adresu IP do postaci czytelnej dla czlowieka: */
        inet_ntop(
            AF_INET,
            &((struct sockaddr_in*)&ifreqptr->ifr_addr)->sin_addr.s_addr,
            address,
            INET_ADDRSTRLEN
        );

        fprintf(stdout, "\n           address: %s, ", address);

        /* Pobranie maski: */
        retval = ioctl(sockfd, SIOCGIFNETMASK, &ifreqstruct);
        if (retval == -1) {
            perror("ioctl()");
            exit(EXIT_FAILURE);
        }

        /* Konwersja maski do postaci czytelnej dla czlowieka: */
        inet_ntop(
            AF_INET,
            &((struct sockaddr_in*)&(ifreqstruct.ifr_addr))->sin_addr.s_addr,
            address,
            INET_ADDRSTRLEN
        );

        fprintf(stdout, "netmask: %s\n", address);
    }

    close(sockfd);
    free(buff);
    exit(EXIT_SUCCESS);
}
