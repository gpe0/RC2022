#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "download.h"

int findChar(char * word, char v) {
    for (int i = 0; i < strlen(word); i++) {
        if (word[i] == v) return i;
    }
    return -1;
}

int substr(char * old, char * new, int start, int end) {
    int j = 0;
    for (int i = start; i < end; i++) {
        new[j++] = old[i];
    }
    return 0;
}

int connectTo(char * address, int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *h;
  
    if ((h = gethostbyname(address)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }

    printf("Connecting to %s\n", h->h_name);  

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *) h->h_addr)));    /*32 bit Internet address network byte ordered*/
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

    printf("Connection successfully!\n");  
    return sockfd;
}

int isCode(char * response, char * code) {
    if (strlen(response) < 3 || strlen(code) < 3) return 1;
    if (response[0] == code[0] && response[1] == code[1] && response[2] == code[2]) return 0;
    return 1;
}

int getDataAddress(char * response, char * address) {
    if (strlen(response) < 48) return 1;
    int i = 27, j = 0, points = 0;
    while (1) {
        if (response[i] == ',') {
            points++;
            if (points == 4) break;
            address[j] = '.';
        }
        else address[j] = response[i];
        j++;
        i++;
    }
    int n1 = 0, n2 = 0;
    int first = 1;
    i++;
    while(1) {
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
        i++;
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

int login(int sockfd, char * user, char * password) {
    /*send a string to the server*/
    size_t bytes;
    char buf[BUF_SIZE] = {0};
    strcat(buf, "user ");
    strcat(buf, user);
    strcat(buf, "\n");
    printf("Login with user : %s and password : %s\n", user, password);
    sendMessage(sockfd, buf);
    sleep(1);
    memset(buf, 0,BUF_SIZE); //clear socket
    
    while(recv(sockfd, buf, BUF_SIZE, MSG_DONTROUTE) > 0) {
        if (strstr(buf, "230 Login successful")) return 0;
        if (strstr(buf, "331 Please specify the password")) break;
        memset(buf, 0,BUF_SIZE); //clear socket
    }
    


    memset(buf, 0,BUF_SIZE); //clear socket

    strcat(buf, "pass ");
    strcat(buf, password);
    strcat(buf, "\n");

    sendMessage(sockfd, buf);
    memset(buf, 0,BUF_SIZE); //clear socket
    sleep(1);
    recv(sockfd, buf, BUF_SIZE, MSG_DONTROUTE);
    if (isCode(buf, "230") == 1) {
        perror("couldn't login");
        exit(-1);
    }
    return 0;
}

int parseInput(char * input, char * user, char * password, char * host, char * file) {
    char cleanInput[BUF_SIZE] = {0};
    substr(input, cleanInput, 6, strlen(input));
    char pathWithSlash[BUF_SIZE] = {0};
    strcpy(pathWithSlash, strstr(cleanInput, "/"));
    if (strstr(input, "@")) {
        substr(cleanInput, user, 0, findChar(cleanInput, ':'));
        substr(cleanInput, password, findChar(cleanInput, ':') + 1, findChar(cleanInput, '@'));
        substr(cleanInput, host, findChar(cleanInput, '@') + 1, findChar(cleanInput, '/'));
        substr(pathWithSlash, file, 1, strlen(pathWithSlash));
    }
    else {
        strcpy(user, "anonymous");
        strcpy(password, "password"); // login creds not provided
        substr(cleanInput, host, 0, findChar(cleanInput, '/'));
        substr(pathWithSlash, file, 1, strlen(pathWithSlash));
    }
    

}

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }
    int sockfdCommands, sockfdData;
    char buf[BUF_SIZE] = {0}, address[BUF_SIZE] = {0}, user[BUF_SIZE] = {0}, password[BUF_SIZE] = {0}, host[BUF_SIZE] = {0}, file[BUF_SIZE] = {0};
    int port;
    size_t bytes;

    parseInput(argv[1], user, password, host, file);

    sockfdCommands = connectTo( host , SERVER_PORT);

    login(sockfdCommands, user, password);

    printf("Entering Passive Mode.\n");
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
    strcat(wantedFile, file);
    strcat(wantedFile, "\n");

    printf("Seeking File...\n");

    sendMessage(sockfdCommands, wantedFile);

    recv(sockfdCommands, buf, BUF_SIZE, MSG_DONTROUTE);

    
    if (isCode(buf, "150") == 1) {
        perror("couldn't find the file");
        exit(-1);
    }
    printf("Downloading...\n");

    sleep(1);


    memset(buf, 0,BUF_SIZE);

    FILE * ptr = fopen("output.txt", "w");

    bytes = recv(sockfdData, buf, BUF_SIZE, MSG_DONTROUTE);

    printf("\n \\/  Data received  \\/\n\n");

    while (bytes > 0) {
        printf("%s", buf);
        fprintf(ptr, "%s", buf);
        memset(buf, 0,BUF_SIZE);
        bytes = recv(sockfdData, buf, BUF_SIZE, MSG_DONTROUTE);
    }
    printf("\n");

    fclose(ptr);

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


