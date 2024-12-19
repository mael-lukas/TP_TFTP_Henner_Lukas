#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAXSIZE 512
#define RRQ 1
#define WRQ 2
#define DAT 3
#define ACK 4
#define ERR 5
#define SERVER_PORT "1069"
#define ACK_BUFF_SIZE 4

int main(int argc, char** argv) {
    // check number of arguments
    if (argc != 3) {
        printf("Invalid number of arguments: expected 3, received %d\r\n",argc);
        exit(EXIT_FAILURE);
    }

    char* host = argv[1];
    char* file = argv[2];

    struct addrinfo *res;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_UDP;

    int status = getaddrinfo(host, SERVER_PORT, &hints, &res);
    if (status == -1) {
        printf("Unable to reach host: %s\r\n",host);
        exit(EXIT_FAILURE);
    }
    int sfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if (sfd == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    printf("created socket\r\n");

    char sendBuffer[MAXSIZE] = {0};

    // filling the send buffer
    // first two bytes used for operation code, here read request
    sendBuffer[0] = 0;
    sendBuffer[1] = WRQ;
    // next bytes are filled with the file name we want to read
    sprintf(sendBuffer + 2,"%s",file);
    // reserved byte
    sendBuffer[2+strlen(file)] = 0;

    sprintf(sendBuffer + 3 + strlen(file), "octet");
    // reserved byte
    sendBuffer[11 + strlen(file)] = 0;
    int sendBufferSize = 9 + strlen(file);


    ssize_t sentBytes = sendto(sfd,sendBuffer,sendBufferSize,0,res->ai_addr,res->ai_addrlen);
    if (sentBytes == -1) {
        printf("Error while sending request\r\n");
        exit(EXIT_FAILURE);
    }
    printf("Read request for %s sent\r\n",file);

    // check for server acknowledgement of write request
    char WRQAckBuffer[ACK_BUFF_SIZE] = {0};
    ssize_t ackRecBytesWRQ = recvfrom(sfd,WRQAckBuffer,ACK_BUFF_SIZE,0,res->ai_addr,&(res->ai_addrlen));
    if (ackRecBytesWRQ == -1) {
        printf("Error while receiving acknowledgement of write request\r\n");
        exit(EXIT_FAILURE);
    }

    char dataToSend[1024] = {0};
    char ackBuffer[ACK_BUFF_SIZE] = {0};
    int sizeOfDataSent = MAXSIZE;
    int sentDataNb;
    int nbOfSplits;

    do {
        int sentDataNb = sendto(sfd,dataToSend,(sizeOfDataSent + 4),0,res->ai_addr,res->ai_addrlen);
        if (sentDataNb == -1) {
            printf("Error while sending data to server\r\n");
            exit(EXIT_FAILURE);
        }
        int ackRecBytes = recvfrom(sfd,ackBuffer,ACK_BUFF_SIZE,0,res->ai_addr,&(res->ai_addrlen));
        if (ackRecBytes == -1) {
            printf("Error while receiving acknowledgement\r\n");
            exit(EXIT_FAILURE);
        }
        if (ackBuffer[0] == 0 && ackBuffer[1] == ACK) {
              nbOfSplits = (ackBuffer[2] << 8) | ackBuffer[3];
        }
    } while(sentDataNb == (MAXSIZE + 4));
}