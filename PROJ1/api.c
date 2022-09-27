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

void timout(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}
int sendSetMessage(int serial)
{
    unsigned char buf[BUF_SIZE] = {FLAG, A2, SET, A2 ^ SET + '0', FLAG, '\0'};
    write(serial, buf, BUF_SIZE);
    sleep(1);
}

int sendUaMessage(int serial)
{
    unsigned char buf[BUF_SIZE] = {FLAG, A2, UA, A2 ^ UA + '0', FLAG, '\0'};
    write(serial, buf, BUF_SIZE);
    sleep(1);
}

int llopen(int serial, unsigned char flag)
{


    (void)signal(SIGALRM, timout);

    unsigned char buf[BUF_SIZE] = {0};

    if (flag == TRANSMITTER)
    {
        while (alarmCount < 4)
        {
            if (alarmEnabled == FALSE)
            {
                sendSetMessage(serial);
                alarm(3);
                alarmEnabled = TRUE;
                memset(buf, 0, BUF_SIZE);
                read(serial, buf, BUF_SIZE);   
                alarm(0); // disable the alarm

                if (buf[0] != FLAG ||
                    buf[1] != A2 ||
                    buf[2] != UA ||
                    buf[3] != (buf[1] ^ buf[2] + '0') ||
                    buf[4] != FLAG)
                {

                    return -1;
                }
                return 0;
            }
        }

        if (alarmCount == 4)
            return -1; // error
    }
    else if (flag == RECEIVER)
    {
        read(serial, buf, BUF_SIZE);


        if (buf[0] != FLAG ||
            buf[1] != A2 ||
            buf[2] != SET ||
            buf[3] != (buf[1] ^ buf[2] + '0') ||
            buf[4] != FLAG)
        {

            return -1;
        }

        sendUaMessage(serial);
    }

    return 0;
}
