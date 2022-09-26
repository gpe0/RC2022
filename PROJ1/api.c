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

// Alarm function handler
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
}

int sendUaMessage(int serial)
{
    unsigned char buf[BUF_SIZE] = {FLAG, A2, UA, A2 ^ UA + '0', FLAG, '\0'};
    write(serial, buf, BUF_SIZE);
}

int llopen(int serial, unsigned char flag)
{
    STOP = FALSE;
    (void)signal(SIGALRM, timout);
    unsigned char buf[BUF_SIZE] = {0};
    unsigned char temp_buf[BUF_SIZE] = {0};
    if (flag == TRANSMITTER)
    {
        sendSetMessage(serial);
        while (STOP == FALSE)
        {
            read(serial, temp_buf, BUF_SIZE);
            strcat(buf, temp_buf);
            if (strlen(temp_buf) == 0)
                STOP = TRUE;
        }

        if (buf[0] != FLAG ||
            buf[1] != A2 ||
            buf[2] != UA ||
            buf[3] != (buf[1] ^ buf[2] + '0') ||
            buf[4] != FLAG)
        {
            return -1;
        }
    }

    else if (flag == RECEIVER)
    {
        while (STOP == FALSE)
        {
            read(serial, temp_buf, BUF_SIZE);
            strcat(buf, temp_buf);
            if (strlen(temp_buf) == 0)
                STOP = TRUE;
            if (alarmCount > 0)
                return -1;
        }

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