#ifndef SERVER_H
#define SERVER_H
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

#define RRQ 1
#define DATA 3
#define ACK 4
#define ERROR 5
#define OCTET "octet"
#define DATALENGTH 512
#define MAX_CLIENTS 4

using namespace std;


void debug_helper(int e )
{
    switch(e)
    {
    case 12:
        cout << "Received invalid tftp_pack. Exception Nr. " << e << '\n';
        break;
    case 13:
        cout << "cannot send invalid tftp_pack. Exception Nr. " << e << '\n';
        break;
    default:
        cout << "recognized error Nr. :"<< e << "\n";
        break;
    }
}

/*In this experiment, the tftp server only expects to receive RRQ and ACK if everything goes correctly*/
typedef struct tftp
{
    uint16_t opcode;
    char * filename;
    char * mode;
    uint16_t blocknumber;
} tftp_pack;

typedef struct
{
    int sockfd;
    struct sockaddr_in address;
    struct addrinfo  address_info;

} connection_info;

//function to reep dead processes (Taken from BEEJ.US)
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}


/*
** from Beej
** packi16() -- store a 16-bit int into a char buffer (like htons())
*/
void packi16(char *buf, unsigned short int i)
{
    // for example i = 0x34d6
    // buffer[0] = 34   buffer[1] = d6
    // Big Endian
    *buf++ = i>>8;
    *buf++ = i;
}

uint16_t unpacki16(char *buf)     //change  network byte order to the host order (16bit)
{
    uint16_t i;
    memcpy(&i,buf,2);  // require <cstring>
    i = ntohs(i);
    return i;
}

/*
* In this experiment, server only expects to send DATA or ERROR
*/
char* encode(uint16_t opcode, uint16_t blocknumber, char* data, int datalength)
{
    char* serialized_packet;
    try
    {
        switch(opcode)
        {
        case DATA:
            //  DATA     :   3(uint16_t)    block number(uint_16)  Data (0 - 512 bytes)
            serialized_packet = (char*)malloc( (datalength + 4) * sizeof(char) );
            packi16(serialized_packet, opcode);
            packi16(serialized_packet + 2, blocknumber);
            strncpy(serialized_packet + 4, data, datalength);
            break;
        case ERROR:
            //  ERROR    :   5(uint_16)    Error number(uint_16)  Error data(variable)         0(1 byte)
            serialized_packet = (char *)malloc( (datalength+5) * sizeof(char));
            packi16(serialized_packet, opcode);
            packi16(serialized_packet + 2, blocknumber);
            strncpy(serialized_packet + 4, data, datalength);
            memset(serialized_packet + 4 + datalength, '\0', 1);
            break;
        default:
            throw 13;
            break;
        }
    }
    catch (int e)
    {
        debug_helper(e);
    }
    return serialized_packet;
}


tftp_pack* decode(char * serialized_packet)
{
    tftp_pack* output;
    output = (tftp_pack*) malloc(sizeof(tftp_pack));
    output -> opcode = unpacki16(serialized_packet + 0);  // cover first two bytes into host short int
    // curent p_position still at head of char array
    try
    {
        switch(output -> opcode)
        {
        case RRQ:
            int index_0,index_1; // index_0 indicate the beginning of filename chunk
            index_1 = index_0 =  2;
            while(serialized_packet[index_1] != '\0')
            {
                //cout << index_1 << " jieshule "<< endl;
                index_1 ++;
            }
            //cout << index_0 << endl;
            //cout << index_1 << endl;
            //  (index_1 + 1 - index_0) == length of '\0'terminated filename char array
            output -> filename = (char * )malloc( (index_1 + 1 - index_0) * sizeof(char) );
            memcpy(output -> filename, serialized_packet+ index_0,  index_1 + 1 - index_0);
            //cout << output -> filename << endl;
            index_0 = index_1;
            while(serialized_packet[index_0] == '\0')
            {
                index_0++;
            }
            // now index_0 should be pointing to beginning of mode chunk
            index_1 = index_0;
            while(serialized_packet[index_1] != '\0')
            {
                //cout << index_1 << " jieshule "<< endl;
                index_1 ++;
            }

            output -> mode = (char * )malloc( (index_1 + 1 - index_0) * sizeof(char) );
            //cout << index_0 << endl;
            //cout << index_1 << endl;
            memcpy(output -> mode, serialized_packet+ index_0,  index_1 + 1 - index_0);
            break;
        case ACK:
            output->blocknumber = unpacki16(serialized_packet + 2);
            break;
        default:
            throw 12;
            break;
        }
    }
    catch (int e)
    {
        debug_helper(e);
    }
    return output;
}



// from page 36 of beej
// get pointer of sin_addr or sin6_addr , IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


// the purpose of this function is get valid socket file descriptor
// return 0, means it is functioning properly
int initialize_server( int argc, char* argv[], connection_info& server_info)
{

    int sockfd;
    if (argc != 3)
    {
        printf ("usage: ./server <ip> <port> \n");
        return 1;
    }
    struct addrinfo hints, *servinfo, *p;

    int status = -1;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;   // use my own ip address

    if ( (status = getaddrinfo(argv[1], argv[2], &hints, &servinfo ) ) != 0 )
    {
        fprintf( stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }
    // servinfo now points to result linked list
    for (p = servinfo; p != NULL; p = p -> ai_next )
    {
        if ( (sockfd = socket(p -> ai_family, p-> ai_socktype, p->ai_protocol) ) == -1)
        {
            perror("unable to listern at this *p");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p-> ai_addrlen) == -1)
        {
            close(sockfd);
            perror("unable to bind this *p");
            continue;
        }
        // no error happened to this *sockfd, which means it is a valid one.
        break;
    }
    if (p == NULL)
    {
        fprintf(stderr, "listerner: failed to bind sockets\n");
        return 2;
    }
    //char ipstr[INET6_ADDRSTRLEN];
    //inet_ntop(AF_INET, &( ( (struct sockaddr_in *)p->ai_addr)->sin_addr), ipstr, sizeof ipstr );
    //printf("listening: %s. \n" , ipstr);

    server_info.sockfd = sockfd;
    //server_conn.address.sin_addr =
    //server_conn.address = *( (struct sockaddr_in *) p->ai_addr);
    server_info.address = *( (struct sockaddr_in *) p->ai_addr);
    server_info.address_info = *p;
    printf("server initialized, going to recvfrom .., \n");

    return 0;
}


void stop_server(connection_info connections[], int server_sockfd)
{

    int i;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        //send();
        close(connections[i].sockfd);
    }
    close(server_sockfd);
    printf("user exit from server\n");
    exit(0);
}


/*
*  Treat file descriptor like a managed piple, how many pipes this server will care about? 4
*  Following keeps put active sockfd into fd_set, to make them managed
*/
int construct_fd_set(fd_set *set, connection_info *server_info, connection_info clients[])
{
    FD_ZERO(set);
    FD_SET(STDIN_FILENO, set);
    FD_SET(server_info->sockfd, set);

    int max_fd = server_info->sockfd;
    int i;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i].sockfd > 0)
        {
            FD_SET(clients[i].sockfd, set);
            if(clients[i].sockfd > max_fd)
            {
                max_fd = clients[i].sockfd;
            }
        }
    }
    return max_fd;
}
void trim_newline(char *text)
{
    int len = strlen(text) - 1;
    if (text[len] == '\n')
    {
        text[len] = '\0';
    }
}


void handle_user_input(connection_info clients[], int server_sockfd)
{
    char input[255];
    fgets(input, sizeof(input), stdin);
    trim_newline(input);

    if(input[0] == 'q')
    {
        stop_server(clients, server_sockfd);
    }
}

void occupyOneClientSlot(struct sockaddr_in their_add, connection_info clients[])
{
    printf("occupy\n");
    return;
}



#endif // SERVER_H
