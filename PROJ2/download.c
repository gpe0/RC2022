#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "download.h"

int connectTo(char * address, int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }
    return sockfd;
}

int isCode(char * response, char * code) {
    if (strlen(response) < 3 || strlen(code) < 3) return 1;
    if (response[0] == code[0] && response[1] == code[1] && response[2] == code[2]) return 0;
    return 1;
}

int getDataAddress(char * response, char * address) {
    if (strlen(response) < 48) return 1;
    int j = 0;
    for (int i = 27; i < 40; i++) {
        if (response[i] == ',') address[j] = '.';
        else address[j] = response[i];
        j++;
    }
    int n1 = 0, n2 = 0;
    int first = 1;
    for (int i = 41; i < 48; i++) {
        if (response[i] == ')') {
            break;
        }
        else if (response[i] == ',') {
            first = 0;
        }
        else if (first == 1) {
            n1 = n1 * 10 + (response[i] - '0');
        }
        else {
            n2 = n2 * 10 + (response[i] - '0');
        }
    }
    return n1 * 256 + n2;
}


int sendMessage(int sockfd, char * message) {
    int bytes = write(sockfd, message, strlen(message));
    if (bytes <= 0) {
        perror("write()");
        exit(-1);
    }
    return 0;
}

int login(int sockfd) {
    /*send a string to the server*/
    size_t bytes;
    char buf[BUF_SIZE];
    sendMessage(sockfd, "user anonymous\n");
    sleep(1);
    recv(sockfd, buf, BUF_SIZE, MSG_DONTROUTE);
    memset(buf, 0,BUF_SIZE); //clear socket

    sendMessage(sockfd, "pass password\n");
    sleep(1);
    recv(sockfd, buf, BUF_SIZE, MSG_DONTROUTE);
    if (isCode(buf, "230") == 1) {
        perror("couldn't login");
        exit(-1);
    }
    return 0;
}

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file to download>\n", argv[0]);
        exit(-1);
    }
    int sockfdCommands, sockfdData;
    char buf[BUF_SIZE];
    char address[BUF_SIZE];
    int port;
    size_t bytes;

    sockfdCommands = connectTo(SERVER_ADDR, SERVER_PORT);

    login(sockfdCommands);

    sendMessage(sockfdCommands, "pasv\n");

    
    recv(sockfdCommands, buf, BUF_SIZE, MSG_DONTROUTE);

    if (isCode(buf, "227") == 1) {
        perror("couldn't enter passive mode");
        exit(-1);
    }

    port = getDataAddress(buf, address);
    
    sockfdData = connectTo(address, port);

    memset(buf, 0,BUF_SIZE);

    char wantedFile[BUF_SIZE] = "retr ";

    strcat(wantedFile, argv[1]);
    strcat(wantedFile, "\n");

    sendMessage(sockfdCommands, wantedFile);

    recv(sockfdCommands, buf, BUF_SIZE, MSG_DONTROUTE);
    
    if (isCode(buf, "150") == 1) {
        perror("couldn't find the file");
        exit(-1);
    }

    memset(buf, 0,BUF_SIZE);
    sleep(1);

    recv(sockfdCommands, buf, BUF_SIZE, MSG_DONTROUTE);
    
    if (isCode(buf, "226") == 1) {
        perror("couldn't finish the download");
        exit(-1);
    }

    memset(buf, 0,BUF_SIZE);

     recv(sockfdData, buf, BUF_SIZE, MSG_DONTROUTE);
    printf("Data received: %s\n", buf);

    if (close(sockfdCommands)<0) {
        perror("close()");
        exit(-1);
    }

    if (close(sockfdData)<0) {
        perror("close()");
        exit(-1);
    }
    return 0;
}


