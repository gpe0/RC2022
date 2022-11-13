#define SERVER_PORT 21
#define SERVER_ADDR "193.137.29.15"
#define BUF_SIZE 1000

int connectTo(char * address, int port);

int isCode(char * response, char * code);

int getDataAddress(char * response, char * address);

int sendMessage(int sockfd, char * message);

int login(int sockfd);

int main(int argc, char **argv);