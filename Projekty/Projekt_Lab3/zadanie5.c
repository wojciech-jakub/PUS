/*
 * Data:                2009-02-27
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc udp.c -o udp
 * Uruchamianie:        $ ./udp <adres IP lub nazwa domenowa> <numer portu>
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netdb.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <netinet/ip_icmp.h>
 #include <unistd.h>
 #include <errno.h>
 #include <string.h>
 #include <time.h>
 #include "checksum.h"

#define SOURCE_PORT 4545
#define SOURCE_ADDRESS "192.168.1.14"




int main(int argc, char** argv) {

    int                     sockfd; /* Deskryptor gniazda. */
    int                     socket_option; /* Do ustawiania opcji gniazda. */
    int                     retval; /* Wartosc zwracana przez funkcje. */
    int                     ttl = 255;

    /* Struktura zawierajaca wskazowki dla funkcji getaddrinfo(): */
    struct addrinfo         hints;

    /*
     * Wskaznik na liste zwracana przez getaddrinfo() oraz wskaznik uzywany do
     * poruszania sie po elementach listy:
     */
    struct addrinfo         *rp, *result;

    /* Zmienna wykorzystywana do obliczenia sumy kontrolnej: */
    unsigned short          checksum;
    unsigned char           data[32];

    /* Bufor na naglowek IP, naglowek UDP oraz pseudo-naglowek: */
    unsigned char           datagram[sizeof(struct icmphdr)];

    struct icmphdr      *icmp_header          =(struct icmphdr*)(datagram);
    if (argc != 2) {
        fprintf(
            stderr,
            "Invocation: %s <HOSTNAME OR IP ADDRESS>\n",
            argv[0]
        );

        exit(EXIT_FAILURE);
    }

    /* Wskazowki dla getaddrinfo(): */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family         =       AF_INET; /* Domena komunikacyjna (IPv4). */
    hints.ai_socktype       =       SOCK_RAW; /* Typ gniazda. */
    hints.ai_protocol       =       IPPROTO_ICMP; /* Protokol. */


    retval = getaddrinfo(argv[1], NULL, &hints, &result);
    if (retval != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(retval));
        exit(EXIT_FAILURE);
    }

    /* Opcja okreslona w wywolaniu setsockopt() zostanie wlaczona: */
    socket_option = 1;

    /* Przechodzimy kolejno przez elementy listy: */
    for (rp = result; rp != NULL; rp = rp->ai_next) {

        /* Utworzenie gniazda dla protokolu UDP: */
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            perror("socket()");
            continue;
        }

        /* Ustawienie opcji IP_HDRINCL: */
        retval = setsockopt(
                     sockfd,
                     IPPROTO_IP, IP_TTL,
                     (const char*)&ttl, sizeof(int)
                 );

        if (retval == -1) {
            perror("setsockopt()");
            exit(EXIT_FAILURE);
        } else {
            /* Jezeli gniazdo zostalo poprawnie utworzone i
             * opcja IP_HDRINCL ustawiona: */
            break;
        }
    }

    /* Jezeli lista jest pusta (nie utworzono gniazda): */
    if (rp == NULL) {
        fprintf(stderr, "Client failure: could not create socket.\n");
        exit(EXIT_FAILURE);
    }

    /********************************/
    /* Wypelnienie pol naglowka ICMP: */
    /********************************/

    int i = 0;
    int j = 0;
      pid_t pid = fork();
      if(pid == 0)
      {
        unsigned char datagram[sizeof(icmphdr) ];
        icmphdr *icmp_header = (icmphdr *)datagram;
        for (i = 0 ; i < 4 ; i++)
        {
          for (j = 0 ; j < 31 ; j++)
          {
            datagram[sizeof(icmphdr) + i]=rand() % 58 + 65;
          }
          datagram[sizeof(icmp_header) + 31] = '\0';

          icmp_header->type = ICMP_ECHO;
          icmp_header->code = 0;
          icmp_header->un.echo.id = htons(getpid());
          icmp_header->un.echo.sequence = htons(i);
          icmp_header->checksum=0;
          icmp_header->checksum = internet_checksum((unsigned short*)datagram,sizeof(datagram));
          retval = sendto(socketDescriptor,(const char*) icmp_header,sizeof(datagram),0,rp->ai_addr,rp->ai_addrlen);
          if(retval==-1)
          {
            perror("sendto()");
          }
          printf("SENDING ICMP REQUEST\n", );
          Sleep(1);

        }
        exit(EXIT_SUCCESS);
      }
      else if (pid > 0)
      {
        unsigned char datagram[sizeof(icmphdr) + dataSize];
        sockaddr_in addresStruct;

        ip *ipheader = (ip*) datagram;
        icmphdr *icmp_header = (icmphdr *)datagram;

        socklen_t addresStructSize = sizeof(addresStruct);
        for(i = 0 ; i < 4 ; i++)
        {
          recvfrom(socketDescriptor, datagram, sizeof(datagram), 0, (sockaddr *) &addresStruct, &addresStructSize);

          printf("Wiadomosc od: %s, TTL= %d, Rozmiar naglowka = %d, Adres Docelowy = %s\n", srcaddr, ipheader->ip_ttl, sizeof(ipheader)*8, dstaddr);
		        printf("ICMP: typ=%d, kod=%d, id=%4.X, nr sekwencyjny=%X\n",icmpheader->type,icmpheader->code,ntohs(icmpheader->un.echo.id),icmpheader->un.echo.sequence);
        }
      }






    exit(EXIT_SUCCESS);
}
