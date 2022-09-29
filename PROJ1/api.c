// Write to serial port in non-canonical mode

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include "api.h"

#define BUF_SIZE 256

volatile int STOP = FALSE;

int alarmEnabled = FALSE;
int alarmCount = 0;


struct termios oldtio;
struct termios newtio;

void timout(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    printf("Alarm #%d\n", alarmCount);
}
int sendSetMessage(int fd)
{
    unsigned char buf[BUF_SIZE] = {FLAG, A2, SET, A2 ^ SET + '0', FLAG, '\0'};
    write(fd, buf, BUF_SIZE);
    sleep(1);
}

int sendUaMessage(int fd)
{
    unsigned char buf[BUF_SIZE] = {FLAG, A2, UA, A2 ^ UA + '0', FLAG, '\0'};
    write(fd, buf, BUF_SIZE);
    sleep(1);
}

int llopen(const char * serial, unsigned char flag)
{

     // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serial, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serial);
        exit(-1);
    }

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 20; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received (now is 0)

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    printf("New termios structure set\n");


    (void)signal(SIGALRM, timout);

    unsigned char buf[BUF_SIZE] = {0};

    if (flag == TRANSMITTER)
    {
        while (alarmCount < 4)
        {
            tcflush(fd, TCIOFLUSH);
            if (alarmEnabled == FALSE)
            {
                alarm(3);
                sendSetMessage(fd);
                alarmEnabled = TRUE;
                memset(buf, 0, BUF_SIZE);
                read(fd, buf, BUF_SIZE);

                if (alarmEnabled == FALSE) continue;
                alarm(0); // disable the alarm

                if (buf[0] != FLAG ||
                    buf[1] != A2 ||
                    buf[2] != UA ||
                    buf[3] != (buf[1] ^ buf[2] + '0') ||
                    buf[4] != FLAG)
                {
                    return -1;
                }
                return fd;
            }
        }
        
        llclose(fd);
        return -1;
    }
    else if (flag == RECEIVER)
    {
        read(fd, buf, BUF_SIZE);


        if (buf[0] != FLAG ||
            buf[1] != A2 ||
            buf[2] != SET ||
            buf[3] != (buf[1] ^ buf[2] + '0') ||
            buf[4] != FLAG)
        {

            return -1;
        }

        sendUaMessage(fd);
    }

    return fd;
}

int llclose(int fd) {
    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    close(fd);
    return 0;
}
