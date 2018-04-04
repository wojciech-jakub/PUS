#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <string.h>
// #include <arpa/inet.h>

int main(int argc, char** argv) {
    struct sockaddr_in6 server;
    int listen_fd;
    socklen_t       server_len;
    
    server_len = sizeof(server);
    


    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if ((listen_fd = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    
    memset(&server, 0, sizeof(server));
    server.sin6_family = AF_INET6;
    server.sin6_addr   = in6addr_any;
    server.sin6_port   = htons(atoi(argv[1]));

    if (bind(listen_fd, (struct sockaddr*) &server, server_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, 10) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server is listening for incoming connection...\n");

    while(1){
        int client_fd;
        struct sockaddr_in6     client;
        socklen_t               client_len;

        memset(&client, 0, sizeof(client));
        client_len = sizeof(client);

        client_fd = accept(listen_fd, (struct sockaddr*)&client, &client_len);

        char *addr_buff;
        inet_ntop(AF_INET6, &(client.sin6_addr), addr_buff, INET6_ADDRSTRLEN);
        fprintf(
            stdout, "TCP connection accepted from %s\tPORT:%d\n",
            addr_buff,
            ntohs(client.sin6_port)
        );
       


        if(IN6_IS_ADDR_V4MAPPED(&client.sin6_addr)){
            fprintf(stdout, "IPv4-mapped IPv6\n\n");
        }else{
            fprintf(stdout, "IPv6\n\n");
        }

        if (client_fd == -1) {
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        write(client_fd, "Laboratorium PUS", 17);
        close(client_fd);

    }

}