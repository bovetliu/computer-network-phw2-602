//
//#include "server_util.h"
#include "task.h"
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
    struct sigaction sa;
    //struct sockaddr_storage client_addr, tmp_addr;
    char ipstr[INET_ADDRSTRLEN];        //INET6_ADDRSTRLEN for IPv6
    connection_info server_conn_info;
    //connection_info *clients_conn_info =(connection_info*)malloc(MAX_CLIENTS*sizeof(connection_info));
    //Task* tasks = (Task*)malloc(MAX_CLIENTS * sizeof(Task));

    deque<Task> tasks; // empy deque tasks
    initialize_server(argc, argv, server_conn_info);
    printf("initial sockfd: %d\n", server_conn_info.sockfd);

    // Initializing the required variables
    char buf[1024];
    int num_bytes;

    struct sockaddr_in new_addr_in, client_addr_in, tmp_addr_in;
    socklen_t addr_len;
    new_addr_in = server_conn_info.address;
    new_addr_in.sin_port = htons(0); // so can have random new available port
    struct tftp *p_rec_tftpR;
    int i; // shared counter
    //Bowei, do not understand here
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    printf("Server Running ...\n");

    while(1){
        fdmax = Task::construct_fd_set(&read_fds, &server_conn_info, &tasks);
        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("Select Failed");
            break;
        }

        if (FD_ISSET(server_conn_info.sockfd, &read_fds)){
            // handle new connection
            addr_len = sizeof client_addr_in;
            if ((num_bytes = recvfrom(server_conn_info.sockfd, buf, 1023 , 0,(struct sockaddr *)&client_addr_in, &addr_len)) == -1) {
                perror("recvfrom");
                exit(1);
            }
            printf("Server: got packet from %s on port %hu\n",
                inet_ntop(client_addr_in.sin_family,get_in_addr((struct sockaddr *)&client_addr_in),ipstr, sizeof ipstr),
                ntohs(client_addr_in.sin_port)
            );
            //printf("Server: packet is %d bytes long\n", num_bytes);
            p_rec_tftpR = decode(buf);  //decode received message
            cout <<"Opcode = "<< p_rec_tftpR->opcode <<", Filename = "<< p_rec_tftpR->filename <<", Mode = "<< p_rec_tftpR->mode <<", Request Number = "<< p_rec_tftpR->blocknumber<< endl;
            if(p_rec_tftpR->opcode != RRQ){
                cout << "Invalid Opcode Received" << endl;
                continue;
            }
            //getting new socket, according beej 38, I do not need bind this sockfd
            int new_sockfd = 0;
            if((new_sockfd = socket(server_conn_info.address_info.ai_family, server_conn_info.address_info.ai_socktype, server_conn_info.address_info.ai_protocol))== -1){   // create the new server socket
                perror("server: socket");
            }
            //printf("new sockfd : %d\n",new_sockfd);
            // now I have new sockfd, client_addr_in, p_rec_tftpR->filename  all three ready
            //( int new_sockfd,struct sockaddr_in * client_addr_in,  char * filename, deque<Task> &tasks)
            Task::handle_new_connection(new_sockfd, &client_addr_in,p_rec_tftpR->filename, &tasks );
        }

        // handle tasks here
        for (i = 0; i< tasks.size(); i++){
            if (tasks.at(i).m_sockfd > 0 && FD_ISSET( tasks.at(i).m_sockfd, &read_fds )){

                addr_len = sizeof tmp_addr_in;
                if ( (num_bytes = recvfrom(tasks.at(i).m_sockfd, buf, 1023, 0, (struct sockaddr *)&tmp_addr_in, &addr_len ) )== -1){
                    perror("recvfrom");
                    exit(1);
                }
                if(tmp_addr_in.sin_addr.s_addr == tasks.at(i).m_cli_addr_in.sin_addr.s_addr){
                    // ip address matches
                    tftp_pack*  preturned_tftp = decode(buf);
                    //printf("  opcode %d  ACK number %d \n",preturned_tftp->opcode, preturned_tftp->blocknumber);
                    tasks.at(i).respond( preturned_tftp);
                }

            }
        }

    }// while (1) loop
    close(server_conn_info.sockfd);
    for(i = 0; i < tasks.size(); i++)
    {
        //send();
        close(tasks.at(i).m_sockfd);
    }
    freeaddrinfo(&server_conn_info.address_info);
    return 0;
}
