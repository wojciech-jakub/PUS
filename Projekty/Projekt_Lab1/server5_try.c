#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <time.h>
#include <errno.h>



void *handle_client_connection(void *arg);                 

int main(int argc, char** argv) {
    int connctedfd, listen_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t server_addr_len, client_addr_len;
    pthread_t child;
    FILE *fp;

    char addr_buff[256];
    

    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    listen_socket = socket(PF_INET, SOCK_STREAM, 0);
    
    if (listen_socket == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    
    server_addr.sin_family          =       AF_INET;
    server_addr.sin_addr.s_addr     =       htonl(INADDR_ANY);
    server_addr.sin_port            =       htons(atoi(argv[1]));
    server_addr_len                 =       sizeof(server_addr);

    if (bind(listen_socket, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_socket, 2) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    
    while (1)                         /* process all incoming clients */
    {
        client_addr_len = sizeof(client_addr);
        connctedfd = accept(listen_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (connctedfd == -1) {
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        fprintf(
            stdout, "TCP connection accepted from %s:%d\n",
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
            ntohs(client_addr.sin_port)
        );


        fp = fdopen(connctedfd, "r+");           /* convert into FILE* */
        pthread_create(&child, 0, handle_client_connection, fp);       /* start thread */
        pthread_detach(child);                      /* don't track it */
    }

    close(listen_socket);
    fprintf(stdout, "Closing server.\n");
    exit(EXIT_SUCCESS);

    return 0;
}    

void *handle_client_connection(void *arg)                    /* handle client connection thread */
{	
    FILE *fp = (FILE*)arg;            /* get & convert the data */
	char s[100];

	   /* proc client's requests */
	while (fgets(s, sizeof(s), fp) != 0  &&  strcmp(s, "bye\n") != 0)
	{
		printf("msg: %s", s);                  /* display message */
		fputs(s, fp);                             /* echo it back */
	}
	fclose(fp);                   /* close the client's channel */
	return 0;                           /* terminate the thread */
}