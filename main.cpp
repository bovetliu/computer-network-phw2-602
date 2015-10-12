
#include "server_util.h"
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
    task tasks[MAX_CLIENTS];
    initialize_server(argc, argv, server_conn_info);
    printf("initial sockfd: %d\n", server_conn_info.sockfd);

    // Initializing the required variables
    char buf[1024];
    int num_bytes;

    struct sockaddr_in new_addr_in, client_addr_in;
    socklen_t addr_len;
    new_addr_in = server_conn_info.address;
    new_addr_in.sin_port = htons(0); // so can have random new available port
    struct tftp *p_rec_tftpR;
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
        fdmax = construct_fd_set(&read_fds, &server_conn_info, tasks);
        if(select(max_fd+1, &read_fds, NULL, NULL, NULL) < 0)
        {
          perror("Select Failed");
          stop_server(tasks, server_conn_info.sockfd);
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
            cout <<"Opcode = "<< p_rec_tftpR->opcode <<", Filename = "<< p_rec_tftpR->filename <<", Mode = "<< p_rec_tftpR->mode << endl;
            if(p_rec_tftpR->opcode != RRQ){
                cout << "Invalid Opcode Received" << endl;
                continue;
            }
            //getting new socket, according beej 38, I do not need bind this sockfd
            int new_sockfd = 0;
            if((new_sockfd = socket(server_conn_info.address_info.ai_family, server_conn_info.address_info.ai_socktype, server_conn_info.address_info.ai_protocol))== -1){   // create the new server socket
                perror("server: socket");
            }
            printf("new sockfd : %d\n",new_sockfd);
            // now I have new sockfd, client_addr_in, p_rec_tftpR->filename  all three ready
            //( int new_sockfd,struct sockaddr_in * client_addr_in,  char * filename, task tasks[])
            handle_new_connection(new_sockfd, &client_addr_in,p_rec_tftpR->filename, tasks );
        }
        do{
            if(last_ack==blocknumber){
                myfile.read(file_buf,512);                  // Read next 512 bytes
                m_len = myfile.gcount();
                blocknumber++;
                resend = 0;
                //cout << "Blocknumber : " << blocknumber << endl;
            }
            if(resend > 0){
                if(resend == 51){
                    cout << "Time Out !! " << endl;
                    break;
                }
                cout << "Resending Attempt " << resend << " for Blocknumber " << blocknumber << endl;
            }
            char *packet = encode(DATA,blocknumber,file_buf,m_len);                       // Encode the data into the TFTP packet structure
            if( (num_bytes = sendto(sockfd,packet,m_len+4,0,(struct sockaddr *)&client_addr_in,addr_len)) == -1 ){ // Send Data Packet
                perror("server: sendto");
                exit(1);
            }
            free(packet);


            if( select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){      // Select between client socket to wait for ACK or timeout after 100 ms
                perror("server: select");
                exit(4);
            }
            if(FD_ISSET(sockfd,&read_fds)){
                struct sockaddr_in tmp_addr_in;
                addr_len = sizeof tmp_addr_in;
                if ( (num_bytes = recvfrom(sockfd, buf, 1023 , 0,(struct sockaddr *)&tmp_addr_in, &addr_len)) == -1 ) {        // receive ACK
                    perror("recvfrom");
                    exit(1);
                }
                if(tmp_addr_in.sin_addr.s_addr == client_addr_in.sin_addr.s_addr){
                    p_rec_tftpR = decode(buf);                                      // decode ACK message
                    // need some conditional block to know whether this is an ACK or ERROR

                    if (p_rec_tftpR->opcode == ACK){
                        last_ack = p_rec_tftpR->blocknumber;
                    } else if (p_rec_tftpR->opcode == ERROR){
                        printf("ERROR: BLOCKNUMBER: %d\n", p_rec_tftpR->blocknumber);
                    }

                }
            }
            resend++;
        }while(last_ack<num_packets);
        /***********************************************************************************/
        myfile.close();
        close(sockfd);
        printf("Request Complete\n");
        printf("------------------------------------------------------------\n");
    }// while (1) loop
    close(server_conn_info.sockfd);
    freeaddrinfo(&server_conn_info.address_info);
    return 0;
}
