#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <string.h>
#include <unistd.h>     /* close() */

int main(int argc, char** argv) {
    struct  sockaddr_in server;
    int                 listen_fd;
    socklen_t           server_len;
    
    server_len = sizeof(server);


    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((listen_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    
    memset(&server, 0, sizeof(server));
    server.sin_family           = AF_INET;
    server.sin_addr.s_addr      = htonl(INADDR_ANY);
    server.sin_port             = htons(atoi(argv[1]));

    if (bind(listen_fd, (struct sockaddr*) &server, server_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, 5) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server is listening for incoming connection...\n");


    while(1){
        int client_fd;
        struct sockaddr_in     client;
        socklen_t              client_len;

        memset(&client, 0, sizeof(client));
        client_len = sizeof(client);

        client_fd = accept(listen_fd, (struct sockaddr*)&client, &client_len);

        char addr_buff[16];
        inet_ntop(AF_INET, &(client.sin_addr), addr_buff, INET_ADDRSTRLEN);
        fprintf(
            stdout, "TCP connection accepted from %s\tPORT:%d\n",
            addr_buff,
            ntohs(client.sin_port)
        );

        write(client_fd, "Laboratorium PUS", 17);
        close(client_fd);
    }



}