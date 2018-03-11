#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <time.h>
#include <errno.h>
#define MAX_CONN 15

int     listenfd, clients[MAX_CONN];
int     N;
fd_set  fds;



char    addr_buff[256];

void checkForConnection();
void checkConnected();
void removeEl(int *array, int index, int length);

int main(int argc, char** argv) {
    int             retval;
    struct          sockaddr_in client_addr, server_addr;
    socklen_t       client_addr_len, server_addr_len;
    char            buff[256];
    int i;
    N=0;

    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family          =       AF_INET;
    server_addr.sin_addr.s_addr     =       htonl(INADDR_ANY);
    server_addr.sin_port            =       htons(atoi(argv[1]));
    server_addr_len                 =       sizeof(server_addr);

    if (bind(listenfd, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 2) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server is listening for incoming connection...\n");

    while(1){
        FD_ZERO(&fds);
        FD_SET(listenfd, &fds);

        for(i=0; i<N;i++){
            FD_SET(clients[i], &fds);
        }

        if (select(sizeof(fds)*8, &fds, NULL, NULL, NULL) > 0){
            checkForConnection();
            checkConnected();
        }else {
            perror("select failed");
            exit(EXIT_FAILURE);
        }

    }
    exit(EXIT_SUCCESS);
}

void checkConnected(){
    int i,j;
    int             retval;
    socklen_t       client_addr_len;

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));

    char            buff[256];
    memset(&buff, 0, sizeof(buff));

    for(i=0; i<N;i++){
        if(FD_ISSET(clients[i], &fds)){

            if (read(clients[i], buff, sizeof(buff)) == 0) {
                fprintf(stdout, "Client disconected \n");

                close(clients[i]);
                FD_CLR(clients[i], &fds);


                removeEl(clients, i, N);
                N--;
            }else {
                // wyslij do wszystkich
                for(j=0; j<N;j++){
                    send(clients[j], buff, strlen(buff),0);
                }
            }

        }
    }
}

void checkForConnection(){
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int newConn;

    memset(&client_addr,0,sizeof(client_addr));

    if(FD_ISSET(listenfd, &fds)){
        client_addr_len = sizeof(client_addr);
        newConn = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (newConn == -1) {
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        // insertArray(&clientList, newConn);
        clients[N++] = newConn;

        fprintf(
            stdout, "TCP connection accepted from %s:%d\n",
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
            ntohs(client_addr.sin_port)
        );
    }

}

void removeEl(int *array, int index, int length){
    int i;
    for(i=index; i<length-1; i++){
        array[i] = array[i+1];
    }
    array[length-1] = -1;
}
