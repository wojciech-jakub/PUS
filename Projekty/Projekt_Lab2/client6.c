#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include <net/if.h> // if_nametoindex()

int main(int argc, char** argv) {
    int socket_fd;
    struct          sockaddr_in6 remote_addr;
    socklen_t       addr_len;
    char            buff[256];
    int retval;
    
    if (argc != 4) {
        fprintf(stderr, "Invocation: %s <IPv6 ADDRESS> <PORT> <INTERFACE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    
    memset(&remote_addr, 0, sizeof(remote_addr));
    inet_pton(AF_INET6, argv[1], &remote_addr.sin6_addr);
    remote_addr.sin6_family = AF_INET6;
    remote_addr.sin6_port = htons(atoi(argv[2]));
    remote_addr.sin6_scope_id = if_nametoindex(argv[3]);    
    


    addr_len = sizeof(remote_addr);
    
    if (connect(socket_fd, (const struct sockaddr*) &remote_addr, addr_len) == -1) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    memset(buff, 0, 256);
    retval = read(socket_fd, buff, sizeof(buff));
    fprintf(stdout, "Received server response: %s\n", buff);

    close(socket_fd);
    exit(EXIT_SUCCESS);

}