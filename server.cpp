
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h>
#include<fcntl.h>

#include<arpa/inet.h>
#include<netinet/in.h>

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include<sys/wait.h>
#include<signal.h>

#include<iostream>
#include<fstream>
#include<string>
#include<cstdlib>

#include "server.h"

#define RRQ 1
#define DATA 3
#define ACK 4
#define ERROR 5
#define OCTET "octet"
#define DATALENGTH 512

// Thanks BEEJ.US for tutorial on how to use SELECT and pack and unpack functions
// Some parts of the code have been taken from BEEJ.US

using namespace std;
int main(int argc, char *argv[]){
    int error;
    struct sigaction sa;
    struct sockaddr_storage client_addr_storage, tmp_addr_storage;
    socklen_t addr_len, tmp_len;
    char ipstr[INET_ADDRSTRLEN];        //INET6_ADDRSTRLEN for IPv6
    connection_info server_conn_info;
    connection_info *clients_conn_info =(connection_info*)malloc(MAX_CLIENTS*sizeof(connection_info));

    initialize_server(argc, argv, server_conn_info);
    int sockfd = server_conn_info.sockfd;


    // Initializing the required variables
    char buf[1024];
    int num_bytes;

    struct sockaddr new_addr;
    new_addr = *(server_conn_info.address_info.ai_addr);

    struct sockaddr_in* p_new_addr_in;
    p_new_addr_in = (struct sockaddr_in*) &new_addr;
    p_new_addr_in->sin_port = htons(0); // so can have random new available port


    //Bowei, do not understand here
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    fd_set file_descriptors;  // temp file descriptor list for select()
    int fdmax = 0;        // maximum file descriptor number


    int i =0;
    printf("Server Running ...\n");
    while(1){
        fdmax = construct_fd_set(&file_descriptors, &server_conn_info, clients_conn_info);
        if (select (fdmax+1, &file_descriptors, NULL, NULL, NULL)<0){
            perror("Select failed.");
            exit(1);
        }
        if(FD_ISSET(STDIN_FILENO, &file_descriptors)) { handle_user_input(clients_conn_info , server_conn_info.sockfd);}

        if (FD_ISSET(server_conn_info.sockfd, &file_descriptors )){
            handle_new_connection(server_conn_info, clients_conn_info, client_addr_storage,buf);
        }
        for(i = 0; i < MAX_CLIENTS; i++)
        {
              if(clients_conn_info[i].sockfd > 0 && FD_ISSET(clients_conn_info[i].sockfd, &file_descriptors))
              {
                //handle_client_message(clients_conn_info, i);
              }
        }


    }// while (1) loop
    close(sockfd);
    freeaddrinfo(&server_conn_info.address_info);  // i need put this at stop server
    return 0;
}
