#define SERVER_PORT 21
#define SERVER_ADDR "193.137.29.15"
#define BUF_SIZE 2000


int findChar(char * word, char v);

int substr(char * old, char * new, int start, int end);

int connectTo(char * address, int port);

int isCode(char * response, char * code);

int getDataAddress(char * response, char * address);

int sendMessage(int sockfd, char * message);

int login(int sockfd, char * user, char * password);

int parseInput(char * input, char * user, char * password, char * host, char * file);

int main(int argc, char **argv);