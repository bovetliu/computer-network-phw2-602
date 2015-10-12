
#include "task.h"


// constructor
Task::Task(){
    m_hehe = 3;
    std::cout << "should not call this constructor!" << std::endl;
}

Task::Task(int sockfd, char* filename, struct sockaddr_in cli_addr_in ){
    m_sockfd = sockfd;
    strcpy(m_filename, filename);
    memcpy ( &m_cli_addr_in, &cli_addr_in, sizeof(cli_addr_in) );
    int num_bytes;
    m_file.open(m_filename,ios::in | ios::binary);
    if(m_file.is_open()==false){
        std::cout << "Could not open requested file " << m_filename << std::endl;
        printf("------------------------------------------------------------\n");
        string message = "**Could not open requested file**";
        char *emsg= (char *)malloc(1024*sizeof(char));
        emsg = strcpy(emsg,message.c_str());
        int emsg_len = strlen(emsg);
        char *epacket = encode(ERROR,1,emsg,emsg_len);
        free(emsg);

        if( (num_bytes = sendto(m_sockfd,epacket,emsg_len+5,0,(struct sockaddr *)&m_cli_addr_in,sizeof(cli_addr_in))) == -1 ){
            perror("server: sendto");
            exit(1);
        }
        exit(0);
    }
    // Calculate the size of the file
    streampos first,last;
    // seekg: Set position in input sequence
    // tellg: Get position in input sequence
    first = m_file.tellg();
    m_file.seekg(0,ios::end);
    last = m_file.tellg();
    int num_packets = ((last - first) /512) + 1;           // Calculate the number of packets to be sent
    m_file.seekg(0,ios::beg); // set the position of input sequence back
    cout << "File Size = " << last - first<< ", Packets = " << num_packets << endl;
    m_blocknumber = 0;
    m_last_ack = 0; m_resent = 0;
}

bool Task::is_same_task( char* filename, struct sockaddr_in cli_addr_in){
    if ( strcmp(filename, m_filename)!= 0 && cli_addr_in.sin_addr.s_addr != m_cli_addr_in.sin_addr.s_addr){
        return false;
    } else return true;
}
