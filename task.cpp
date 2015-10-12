
#include "task.h"


// constructor
Task::Task(){
    m_hehe = 3;
    m_len = 0;
    m_num_packets = 0;
    m_sockfd = 0;           // important
    strcpy(m_filename,"due ni lao mu");
    m_blocknumber = 0;
    m_last_ack = 0;
    m_resent = 0;
    strcpy(file_buf, "default file buf");

    std::cout << "should not call this constructor!" << std::endl;
}

Task::Task(int sockfd, char* filename, struct sockaddr_in cli_addr_in ){
    m_sockfd = sockfd;
    strcpy(m_filename, filename);
    memcpy ( &m_cli_addr_in, &cli_addr_in, sizeof(cli_addr_in) );

}
Task::Task(const Task & that){
    m_sockfd = that.m_sockfd;
    strcpy(m_filename, that.m_filename);
    memcpy ( &m_cli_addr_in, &(that.m_cli_addr_in), sizeof(m_cli_addr_in) );

}
void Task::init_package(){
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

        if( (num_bytes = sendto(m_sockfd,epacket,emsg_len+5,0,(struct sockaddr *)&m_cli_addr_in,sizeof(m_cli_addr_in))) == -1 ){
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
    m_num_packets = ((last - first) /512) + 1;           // Calculate the number of packets to be sent
    m_file.seekg(0,ios::beg); // set the position of input sequence back
    cout << "File Size = " << last - first<< ", Packets = " << m_num_packets << endl;
    m_blocknumber = 0;
    m_last_ack = 0; m_resent = 0;

    if (m_last_ack == m_blocknumber){
        m_file.read(file_buf, 512);
        m_len = m_file.gcount();
        //printf("m_len %d\n", m_len);
        m_blocknumber ++;
        m_resent = 0;
    }
    if (m_resent >0){
        if (m_resent == 51){
            cout << "Time out !!" << endl;
            close(m_sockfd);
            m_sockfd = 0;
        }
    }
    char *packet = encode(DATA,m_blocknumber ,file_buf,m_len);
    if( (num_bytes = sendto(m_sockfd,packet,m_len+4,0,(struct sockaddr *)&m_cli_addr_in,sizeof(m_cli_addr_in))) == -1 ){ // Send Data Packet
        perror("server: sendto");
        exit(1);
    }
    m_resent ++;
    free(packet);
    printf("finished sending packet numbered %d\n", m_blocknumber);
}
bool Task::is_same_task( char* filename, struct sockaddr_in cli_addr_in){
    if ( strcmp(filename, m_filename)!= 0 && cli_addr_in.sin_addr.s_addr != m_cli_addr_in.sin_addr.s_addr){
        return false;
    } else return true;
}

void Task::respond(struct tftp* returned_pack){

    int num_bytes;
    if (returned_pack->opcode == ACK){

        m_last_ack = returned_pack->blocknumber;

        if (m_last_ack == m_num_packets){
            m_file.close();
            close(m_sockfd);
            printf("Request Complete\n");
            printf("=================================\n");
            m_sockfd = 0; // means this one can be occupied by other now
            return;
        }

    } else if (returned_pack->opcode == ERROR) {
        printf("ERROR: BLOCKNUMBER: %d\n", returned_pack->blocknumber);
    }

    if (m_last_ack == m_blocknumber){

        m_file.read( file_buf, 512);  // updating file_buf
        m_len = m_file.gcount();
        //printf("m_len %d\n", m_len);
        m_blocknumber++;
        m_resent = 0;
    }
    if (m_resent > 0){
        if(m_resent == 51){
            cout << "Time Out !! " << endl;
            close(m_sockfd);
            m_sockfd = 0;
        }
        m_resent ++;
        cout << "Resending Attempt " << m_resent << " for Blocknumber " << m_blocknumber << endl;
    }
    char *packet = encode(DATA,m_blocknumber,file_buf,m_len);
    //printf("   respond: m_blocknumber: %d \n", m_blocknumber);
    if( (num_bytes = sendto(m_sockfd,packet,m_len+4,0,(struct sockaddr *)&m_cli_addr_in,sizeof(m_cli_addr_in))) == -1 ){ // Send Data Packet
        perror("server: sendto");
        exit(1);
    }
    m_resent ++;
    free(packet);
}

Task& Task::operator=(const Task &rhs){

    m_len = rhs.m_len;
    m_num_packets = rhs.m_num_packets;
    m_sockfd = rhs.m_sockfd;           // important
    strcpy(m_filename, rhs.m_filename);
    m_file.open(m_filename, ios::in | ios::binary);
    memcpy ( &m_cli_addr_in, &(rhs.m_cli_addr_in), sizeof(rhs.m_cli_addr_in) );
    m_blocknumber = rhs.m_blocknumber;
    m_last_ack = rhs.m_last_ack;
    m_resent = rhs.m_resent;

    strcpy(file_buf, rhs.file_buf);
    return *this;
}
 //static method
int Task::construct_fd_set(fd_set *set, connection_info *server_info, deque<Task> *p_tasks ){
    FD_ZERO(set);
    FD_SET(STDIN_FILENO, set);
    FD_SET(server_info->sockfd, set);

    int max_fd = server_info->sockfd;
    int i;

    for (i = 0; i < p_tasks->size(); i++){
        if( p_tasks->at(i).m_sockfd > 0)
        {
            FD_SET(p_tasks->at(i).m_sockfd , set);
            if( p_tasks->at(i).m_sockfd > max_fd)
            {
                max_fd = p_tasks->at(i).m_sockfd;
            }
        }
    }
    return max_fd;
}

//static method
void Task::handle_new_connection( int new_sockfd,struct sockaddr_in * client_addr_in,  char * filename, deque<Task> *p_tasks ){
    if (new_sockfd < 0){
        perror("Accept failed");
        exit(1);
    }

    //p_tasks->at(i)
    deque<Task>::iterator it = p_tasks->begin();
    while(it != p_tasks->end()){
        if (it->m_sockfd == 0 ){
            p_tasks->erase(it);
            continue;
        }
        it++;
    }

    Task task(new_sockfd, filename, *client_addr_in);
    p_tasks->push_front(task);
    p_tasks->front().init_package();
    printf("new connection use sockfd, %d\n", p_tasks->front().m_sockfd);
}
