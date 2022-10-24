// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

#define FALSE 0
#define TRUE 1


#define BUF_SIZE 2000

#define FLAG 0x7E
#define A1 0x01
#define A2 0x03
#define SET 0x03
#define UA 0x01
#define DISC 0x0B

#define CONTROL_FRAME_SIZE 5

#define ESC 0x7D //FLAG -> ESC 0x5E
                 //ESC -> ESC 0x5D


#define START_ST 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP_ST 5
#define RESEND 6

#define I_0 0x00
#define I_1 0xB0
#define RR_0 0x05
#define RR_1 0xB5
#define REJ_0 0x01
#define REJ_1 0x81

void timout(int signal);

int llopen(LinkLayer linkOptions);

int llclose(int fd, int showStats);

int llwrite(int fd, unsigned char * buffer, int length);

int llread(int fd, unsigned char * buffer);

int sendSetMessage(int fd);

int sendUaMessage(int fd);

int sendDiscMessage(int fd);

int sendIMessage(int fd, unsigned char * buffer, int length);

int sendRRMessage(int fd);

int sendLastRRMessage(int fd);

int sendREJMessage(int fd);

int receiveMessage(int fd, unsigned char * buffer, int bufSize);

int stateMachine(unsigned char byte, int cState, unsigned char C);

int copyArray(unsigned char * source, unsigned char * dest, unsigned int length);

void clearBuffer(int fd);

#endif
