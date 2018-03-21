#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <time.h>
#include "checksum.h"

void messageRand(int size, char * message){
	srand(time(0));
        const char * let = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";//62
	int r = 0;
	int i;
	for(i = size; i < 63; i++){
		r = rand() % 62;
		message[i] = let[r];
		/*message[i] = 'e';*/
	}
	message[63] = '\0';

}



int main(int argc, char** argv){

    int i;

    int        ttl     =     128;
    int        sockfd;               /* Deskryptor gniazda. */
    int        retval;               /* Wartosc zwracana przez funkcje. */
    int        pid;

    const int icmp_size = sizeof(icmphdr);
    struct addrinfo        *rp, *result;
    struct addrinfo         hints;


    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <HOSTNAME OR IP ADDRESS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family         =       AF_INET; /* Domena komunikacyjna (IPv4). */
    hints.ai_socktype       =       SOCK_RAW; /* Typ gniazda. */
    hints.ai_protocol       =       IPPROTO_ICMP; /* Protokol. */

    retval = getaddrinfo(argv[1], NULL, &hints, &result);
    if (retval != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(retval));
        exit(EXIT_FAILURE);
    }


    /* Przechodzimy kolejno przez elementy listy: */
    for (rp = result; rp != NULL; rp = rp->ai_next) {

        /* Utworzenie gniazda dla protokolu ICMP: */
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            perror("socket()");
            continue;
        }

        /* Ustawienie opcji IP: */
        retval = setsockopt(
                     sockfd, IPPROTO_IP, IP_TTL,
                     &ttl, sizeof(int)
                 );

        if (retval == -1) {
            perror("setsockopt()");
            exit(EXIT_FAILURE);
        } else {
            /* Jezeli gniazdo zostalo poprawnie utworzone i
             * opcje IP ustawione: */
            break;
        }
    }

    if (rp == NULL) {
        fprintf(stderr, "Client failure: could not create socket.\n");
        exit(EXIT_FAILURE);
    }

    pid = fork();


    if ( pid == 0 ) {
        for (i = 0; i < 4; i++){
            char datagram[63];
            memset(datagram, 0, 63);
            icmphdr* icmp_header = (icmphdr*) datagram;
            messageRand(icmp_size, datagram);

            /* Wypelnienie pol naglowka ICMP Echo: */
            srand(time(NULL));
            /* Typ komunikatu: */
            icmp_header->type                =       ICMP_ECHO;
            /* Kod komunikatu: */
            icmp_header->code                =       0;
            /* Identyfikator: */
            icmp_header->un.echo.id          =       htons(getpid());
            /* Numer sekwencyjny: */
            icmp_header->un.echo.sequence    =       htons(i); // zwieksza wraz z petla
            /* Suma kontrolna (plik checksum.h): */
            icmp_header->checksum            =       internet_checksum(
                                                        (unsigned short *)datagram,
                                                        sizeof(datagram));
            retval = sendto( sockfd, (const char*) icmp_header, sizeof(datagram), 0, rp->ai_addr, rp->ai_addrlen );
            if (retval == -1) {
                 perror("sentdo()");
                 exit(EXIT_FAILURE);
            }
            //fprintf(stdout, "Send ICMP message...\n");
            sleep(2);
        }
        exit(EXIT_SUCCESS);

    }else if( pid > 0 ){
        //printf("Proces potomny\n ");
        char datagram[63];

        sockaddr_in adr_struct;
        ip* ip_header = ( ip *) datagram;
        icmphdr* icmp_header = (icmphdr*) (datagram + sizeof(ip));
        socklen_t adr_struct_size  = sizeof( adr_struct );

        for(i = 0; i < 4; i++){
            recvfrom    (sockfd,
                        datagram,
                        sizeof(datagram),
                        0,
                        (sockaddr*) &adr_struct,
                        &adr_struct_size );
            printf("\nOdebrano: \n");
            printf("Adres hosta: \t\t %s \n", inet_ntoa(ip_header->ip_src));
            printf("TTL pakietu: \t\t %d \n", ip_header->ip_ttl);
            printf("Dlugosc naglowa: \t %d\n", ip_header->ip_hl);
	    printf("Adres docelowy: \t %s \n", inet_ntoa(ip_header->ip_dst));
            printf("ICMP info: \n");
            printf("Type: \t\t\t %d\n", (int)icmp_header->type);
            printf("Code: \t\t\t %d\n", icmp_header->code);
            printf("ID: \t\t\t %d\n", icmp_header->un.echo.id);
            printf("Nr sekewncyjny: \t %d\n\n", icmp_header->un.echo.sequence);
        }
        exit(EXIT_SUCCESS);
    }else{
        perror("fork()");
        exit(EXIT_FAILURE);
    }







	return 0;
}
