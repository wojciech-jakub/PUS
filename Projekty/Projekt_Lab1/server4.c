/*
 * Data:                2009-02-10
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc server1.c -o server1
 * Uruchamianie:        $ ./server1 <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <time.h>
#include <errno.h>

#define MAX_CLIENTS 10

int main(int argc, char** argv) {


    int connctedfd[MAX_CLIENTS], listen_socket;
    int num = 0;
    int i,k;
    int return_value;
    int curr_writing_client;
    int exit_flag = 0;

    struct sockaddr_in server_addr,client_addr;
    socklen_t server_addr_len, client_addr_len;

    char buff[256];
    char addr_buff[256];
    fd_set fds;

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


    while(1)
    {
      FD_ZERO(&fds);
      FD_SET(listen_socket, &fds);
      for(i = 0 ; i < num; ++i)
        FD_SET(connctedfd[i], &fds);

      if(select(sizeof(fds) * 8, &fds, NULL, NULL, 0) > 0)
      {
        if(FD_ISSET(listen_socket, &fds))
        {
          client_addr_len = sizeof(client_addr);
          connctedfd[num] = accept(listen_socket, (struct sockaddr*)&client_addr, &client_addr_len);
          if (connctedfd[num] == -1) {
              perror("accept()");
              exit(EXIT_FAILURE);
          }

          fprintf(
              stdout, "TCP connection accepted from %s:%d\n",
              inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
              ntohs(client_addr.sin_port)
          );

          ++num;
        }

          for(i = 0; i < num ;++i)
          {
            if(FD_ISSET(connctedfd[i], &fds))
            {
              memset(buff, '\0', sizeof(buff));
              return_value = read(connctedfd[i], buff, sizeof(buff));
              curr_writing_client = i;

              if(return_value == 0)
              {
                close(connctedfd[i]);

                for(k = i; k < num - 1; ++k)
                {
                  connctedfd[k] = connctedfd[k + 1];
                }
                num--;
                fprintf(stdout, "Client disconnected. Connected clients:%d\n", num);
                if(num == 0)
                  exit_flag = 1;
              }
              else
              {
                for(i = 0 ; i < num; ++i)
                {
                  if(i != curr_writing_client)
                    write(connctedfd[i], buff, sizeof(buff));
                }
              }
            }
          }
        }
        if(exit_flag)
          break;
      }

    close(listen_socket);
    fprintf(stdout, "Closing server.\n");
    exit(EXIT_SUCCESS);
}
