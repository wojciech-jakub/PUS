#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <netdb.h> /* getaddrinfo() */


#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include <net/if.h> // if_nametoindex()

int main(int argc, char** argv) {
    struct addrinfo hints;
    struct addrinfo         *result, *rp;
    int                     retval;
    int                     ip_version;
    char                    address[NI_MAXHOST];
    socklen_t               sockaddr_size, client_len;
    char            buff[256];
    int socket_fd;
    char host[NI_MAXHOST], serv[NI_MAXSERV];

    
    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IPv4 or IPv6> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family         =       AF_UNSPEC;
    hints.ai_socktype       =       SOCK_STREAM;
    hints.ai_protocol       =       IPPROTO_TCP;
    hints.ai_flags = 0;

    memset(&result, 0, sizeof(result));
    if ((retval = getaddrinfo(argv[1], argv[2], &hints, &result)) != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(retval));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
         if (rp->ai_family == AF_INET) { /* IPv4 */
            ip_version = 4;
            printf("IPv4 socket\n");
            sockaddr_size = sizeof(struct sockaddr_in);
        } else { /* IPv6 */
            ip_version = 6;
            printf("IPv6 socket\n");
            sockaddr_size = sizeof(struct sockaddr_in6);
        }

        socket_fd = socket(rp->ai_family, rp->ai_socktype,
                       rp->ai_protocol);
        
        if (connect(socket_fd, rp->ai_addr, rp->ai_addrlen) == -1) {
            perror("connect()");
            exit(EXIT_FAILURE);
        }

        
        struct sockaddr_storage server;
        socklen_t server_len;

        server.ss_family = rp->ai_family;
        server_len = sizeof(server);
        client_len = sizeof(struct sockaddr_storage);

        char ip_addr[NI_MAXHOST];
        char port_num[NI_MAXSERV];

        getsockname(socket_fd, (struct sockaddr*)&server, &server_len);
        getnameinfo((struct sockaddr*)&server,
                    server_len,
                    ip_addr,
                    NI_MAXHOST,
                    port_num,
                    NI_MAXSERV,
                    NI_NUMERICHOST | NI_NUMERICSERV);
        
        printf("Connected to: %s port: %s\n", ip_addr, port_num);

        memset(buff, 0, 256);
        retval = read(socket_fd, buff, sizeof(buff));
        fprintf(stdout, "Received server response: %s\n", buff);
    }

    if (result != NULL) {
        freeaddrinfo(result);
    }
    
    close(socket_fd);
    exit(EXIT_SUCCESS);

}