
#ifndef TASK_H
#define TASK_H
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
#include "server_util.h"
#define RRQ 1
#define DATA 3
#define ACK 4
#define ERROR 5
#define OCTET "octet"
#define DATALENGTH 512
#define MAX_CLIENTS 4

using namespace std;

class Task{
    private:
    int m_hehe;
    public:
    int m_len;
    int m_num_packets;
    int m_sockfd = 0;           // important
    char m_filename[1024];  // important
    struct sockaddr_in m_cli_addr_in;  // important
    ifstream m_file;
    int m_blocknumber;
    int m_last_ack;
    int m_resent;
    char file_buf[513];

    Task();
    Task(int sockfd, char* filename, struct sockaddr_in cli_addr_in );
    Task(const Task&);

    bool is_same_task( char* filename, struct sockaddr_in cli_addr_in);
    void respond (struct tftp* resondpack );
    void init_package();
    Task& operator=(const Task &rhs);

    static int construct_fd_set(fd_set *set, connection_info *server_info, deque<Task> *tasks );

    static void handle_new_connection( int new_sockfd,struct sockaddr_in * client_addr_in,  char * filename,  deque<Task> *tasks);

};

#endif
