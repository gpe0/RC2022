#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define RECEIVER 0
#define TRANSMITTER 1

#define FLAG 0x7E
#define A1 0x01
#define A2 0x03
#define SET 0x03
#define UA 0x01
#define DISC 0x0B

void timout(int signal);

int llopen(int serial, unsigned char flag);

int sendSetMessage(int serial);

int sendUaMessage(int serial);
