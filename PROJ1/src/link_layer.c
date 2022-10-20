#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include "link_layer.h"

int alarmEnabled = FALSE;
int alarmDetroyed = FALSE;
int alarmCount = 0;

int signalMessage = 0;
unsigned char role;

struct termios oldtio;
struct termios newtio;

LinkLayer linkLayer;


void timout(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    printf("Alarm #%d\n", alarmCount);
    if (alarmCount == 4) alarmDetroyed = TRUE;
}
int sendSetMessage(int fd)
{
    unsigned char buf[BUF_SIZE] = {FLAG, A2, SET, (A2 ^ SET), FLAG};
    write(fd, buf, CONTROL_FRAME_SIZE);
    return 0;
}

int sendUaMessage(int fd)
{
    unsigned char buf[BUF_SIZE] = {FLAG, A2, UA, (A2 ^ UA), FLAG};
    write(fd, buf, CONTROL_FRAME_SIZE);
    return 0;
}

int sendDiscMessage(int fd) {
    unsigned char buf[BUF_SIZE] = {FLAG, A2, DISC, (A2 ^ DISC), FLAG};
    write(fd, buf, CONTROL_FRAME_SIZE);
    return 0;
}

int sendRRMessage(int fd)
{
    unsigned char RRField;
    if (signalMessage == 0)
        RRField = RR_1;
    else
        RRField = RR_0;
    unsigned char buf[BUF_SIZE] = {FLAG, A2, RRField, (A2 ^ RRField), FLAG};
    write(fd, buf, CONTROL_FRAME_SIZE);
    return 0;
}

int sendLastRRMessage(int fd) {
    unsigned char RRField;
    if (signalMessage == 0)
        RRField = RR_0;
    else
        RRField = RR_1;
    unsigned char buf[BUF_SIZE] = {FLAG, A2, RRField, (A2 ^ RRField), FLAG};
    write(fd, buf, CONTROL_FRAME_SIZE);
    return 0;
}

int sendREJMessage(int fd)
{
    unsigned char REJField;
    if (signalMessage == 0)
        REJField = REJ_0;
    else
        REJField = REJ_1;
    unsigned char buf[BUF_SIZE] = {FLAG, A2, REJField, (A2 ^ REJField), FLAG};
    write(fd, buf, CONTROL_FRAME_SIZE);
    return 0;
}

int sendIMessage(int fd, unsigned char *buffer, int length)
{
    unsigned char IField;
    if (signalMessage == 0)
        IField = I_0;
    else
        IField = I_1;
    unsigned char frame[BUF_SIZE] = {FLAG, A2, IField, (A2 ^ IField)};
    unsigned char bcc2;

    unsigned int nextByte = 4; // next byte in the frame

    for (unsigned int i = 0; i < length; i++)
    {
        if (buffer[i] == FLAG) // Byte stuffing
        {
            frame[nextByte++] = ESC;
            if (i == 0)
                bcc2 = ESC;
            else
                bcc2 ^= ESC;

            frame[nextByte++] = 0x5E;

            bcc2 ^= 0x5E;
        }
        else if (buffer[i] == ESC) // Byte stuffing
        {
            frame[nextByte++] = ESC;
            if (i == 0)
                bcc2 = ESC;
            else
                bcc2 ^= ESC;

            frame[nextByte++] = 0x5D;

            bcc2 ^= 0x5D;
        }
        else
        {
            frame[nextByte++] = buffer[i];

            if (i == 0)
                bcc2 = buffer[i];
            else
                bcc2 ^= buffer[i];
        }
    }

    frame[nextByte++] = bcc2;
    frame[nextByte++] = FLAG;
    write(fd, frame, nextByte);
    return 0;
}

int receiveMessage(int fd, unsigned char *buffer)
{
    unsigned char IField;
    unsigned char temp_buf[BUF_SIZE] = {0};

    if (signalMessage == 0)
        IField = I_0;
    else
        IField = I_1;


    if (buffer[0] != FLAG || buffer[1] != A2 || buffer[2] != IField || buffer[3] != (A2 ^ IField)) {
        if (buffer[2] == DISC) return -2;
        if (buffer[3] != IField) return -3;
        return -1;
    }
        

    int hadESC = FALSE;
    unsigned char bcc2;
    int length = 0;
    int i = 4;
    int nextByte = 0;
    while (1)
    {
        
        if (buffer[i + 1] == FLAG && buffer[i + 2] == '\0') {
            if (buffer[i] != bcc2) return -1;
            break;
        }
        else if (i == 4)
            bcc2 = buffer[i];
        else
            bcc2 ^= buffer[i];

        if (hadESC == TRUE)
        {
            if (buffer[i] == 0x5E)
                temp_buf[nextByte++] = FLAG;
            else if (buffer[i] == 0x5D)
                temp_buf[nextByte++] = ESC;
            else
                return -1;
            hadESC = FALSE;
            length++;
        }
        else if (buffer[i] == ESC)
            hadESC = TRUE;
        else
        {
            length++;
            temp_buf[nextByte++] = buffer[i];
            hadESC = FALSE;
        }
        i++;
    }
    if (buffer[++i] != FLAG) {
        return -1;
    }
    copyArray(temp_buf, buffer, nextByte);
    return length;
}

int copyArray(unsigned char *source, unsigned char *dest, unsigned int length)
{

    for (int i = 0; i < length; i++)
    {
        dest[i] = source[i];
    }
    return 0;
}

int stateMachine(unsigned char byte, int cState, unsigned char C)
{
    unsigned char REJField;
    if (signalMessage == 0)
        REJField = REJ_0;
    else
        REJField = REJ_1;

    unsigned char RRField;
    if (signalMessage == 0)
        RRField = RR_1;
    else
        RRField = RR_0;

    switch (cState)
    {
    case START_ST:
        if (byte == FLAG)
        {
            return FLAG_RCV;
        }
        else
        {
            return START_ST;
        }
    case FLAG_RCV:
        if (byte == FLAG)
        {
            return FLAG_RCV;
        }
        if (byte == A2)
        {
            return A_RCV;
        }
        else
        {
            return START_ST;
        }
    case A_RCV:
        if (byte == FLAG)
        {
            return FLAG_RCV;
        }
        if (byte == C)
        {
            return C_RCV;
        }
        if (byte == REJField && C == RRField)
        {
            return RESEND;
        }
        else
        {
            return START_ST;
        }
    case C_RCV:
        if (byte == FLAG)
        {
            return FLAG_RCV;
        }
        if (byte == (C ^ A2))
        {
            return BCC_OK;
        }
        else
        {
            return START_ST;
        }
    case BCC_OK:
        if (byte == FLAG)
        {
            return STOP_ST;
        }
        else
        {
            return START_ST;
        }
    case RESEND:
        return RESEND;
    default: 
        return START_ST;
    }
}

int llopen(LinkLayer linkOptions)
{
    linkLayer = linkOptions;
    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(linkLayer.serialPort, O_RDWR | O_NOCTTY);


    alarmCount = 0;
    alarmEnabled = FALSE;

    if (fd < 0)
    {
        perror(linkLayer.serialPort);
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

    newtio.c_cflag = linkLayer.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 1; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;   // Blocking read until 5 chars received (now is 0)

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

    if (linkLayer.role == LlTx)
    {
        while (alarmCount < linkLayer.nRetransmissions)
        {
            if (alarmEnabled == FALSE)
            {
                alarm(linkLayer.timeout);
                sendSetMessage(fd);
                alarmEnabled = TRUE;
                memset(buf, 0, BUF_SIZE);
                if (alarmEnabled == FALSE)
                    continue;

                int state = START_ST;
                while (read(fd, buf, 1) == 0) {
                    if (alarmEnabled == FALSE)
                        break;
                }
                while (state != STOP_ST)
                {
                    state = stateMachine(buf[0], state, UA);
                    if (alarmEnabled == FALSE)
                        break;
                    read(fd, buf, 1);
                }
                if (alarmEnabled == FALSE)
                    continue;

                alarm(0); // disable the alarm
                printf("Connection stablished!\n");
                return fd;
            }
        }
        llclose(fd);
        return -1;
    }
    else if (linkLayer.role == LlRx)
    {
        int received = FALSE;
        while (received == FALSE)
        {
            int state = START_ST;
            while (read(fd, buf, 1) != 0 && state != STOP_ST)
            {
                state = stateMachine(buf[0], state, SET);
            }
            if (state == STOP_ST)
                received = TRUE;
        }

        sendUaMessage(fd);
    }
    printf("Connection stablished!\n");
    return fd;
}

int llclose(int fd)
{
    (void)signal(SIGALRM, timout);
    alarmCount = 0;
    alarmEnabled = FALSE;
    unsigned char buf[BUF_SIZE] = {0};

    if (linkLayer.role == LlTx) {
        while (alarmCount < linkLayer.nRetransmissions)
        {
            if (alarmEnabled == FALSE)
            {
                sendDiscMessage(fd);
                alarm(linkLayer.timeout);
                int state = START_ST;
                while (read(fd, buf, 1) != 0 && state != STOP_ST)
                {
                    state = stateMachine(buf[0], state, DISC);
                    if (alarmEnabled == FALSE)
                        break;
                }
                alarm(0);
                sendUaMessage(fd);
                break;
            }
        }
    }
    else {
        int state = START_ST;
        while (read(fd, buf, 1) != 0 && state != STOP_ST)
        {
            state = stateMachine(buf[0], state, DISC);
            if (alarmEnabled == FALSE)
                break;
        }
        state = START_ST;
        sendDiscMessage(fd);
        while (read(fd, buf, 1) != 0 && state != STOP_ST)
        {
            state = stateMachine(buf[0], state, UA);
            if (alarmEnabled == FALSE)
                break;
        }
    }


    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }
    close(fd);
    printf("Connection closed!\n");
    return 0;
}

int llwrite(int fd, unsigned char *buffer, int length)
{
    alarmCount = 0;
    alarmEnabled = FALSE;
    (void)signal(SIGALRM, timout);
    unsigned char buf[BUF_SIZE] = {0};

    while (alarmCount < linkLayer.nRetransmissions && alarmDetroyed == FALSE)
    {
        if (alarmEnabled == FALSE)
        {
            alarm(linkLayer.timeout);
            sendIMessage(fd, buffer, length);
            alarmEnabled = TRUE;
            memset(buf, 0, BUF_SIZE);
            if (alarmEnabled == FALSE)
                continue;

            int state = START_ST;
            unsigned char RRField;
            if (signalMessage == 0)
                RRField = RR_1;
            else
                RRField = RR_0;
            while (read(fd, buf, 1) == 0) {
                if (alarmEnabled == FALSE)
                    break;
            }
            do
            {
                state = stateMachine(buf[0], state, RRField);
                if (alarmEnabled == FALSE || state == RESEND)
                    break;
                read(fd, buf, 1);
            } while (state != STOP_ST);

            alarm(0); // disable the alarm

            if (alarmEnabled == FALSE || state == RESEND)
            {
                alarmEnabled = FALSE;
                continue;
            }
            if (signalMessage == 0)
                signalMessage = 1;
            else
                signalMessage = 0;
            return length;
        }
    }
    alarmDetroyed = TRUE;
    return -1;
}

int llread(int fd, unsigned char *buffer)
{
    unsigned char buf[BUF_SIZE] = {0};
    unsigned char temp_buf[BUF_SIZE] = {0};
    int bytes = 0;
    int nextByte = 0;
    int started = FALSE;
    //sleep(5);
    while (read(fd, temp_buf, 1) == 0);
    //sleep(5);
    do
    {
        if (started == FALSE && temp_buf[0] == FLAG) started = TRUE;
        else if (started == TRUE && temp_buf[0] == FLAG) {
            buf[nextByte++] = temp_buf[0];
            break;
        }
        buf[nextByte++] = temp_buf[0];
    } while (read(fd, temp_buf, 1) != 0);
    //sleep(5);
    bytes = receiveMessage(fd, buf);
    //sleep(5);
    if (bytes == -3) {
        sendLastRRMessage(fd);
        nextByte = 0;
        memset(buf, 0, BUF_SIZE);
        while (read(fd, temp_buf, 1) == 0);
        do
        {
            buf[nextByte++] = temp_buf[0];
        } while (read(fd, temp_buf, 1) != 0);
        bytes = receiveMessage(fd, buf);
    }
    while (bytes == -1)
    {
        sendREJMessage(fd);
        nextByte = 0;
        memset(buf, 0, BUF_SIZE);
        while (read(fd, temp_buf, 1) == 0);
        do
        {
            buf[nextByte++] = temp_buf[0];
        } while (read(fd, temp_buf, 1) != 0);
        bytes = receiveMessage(fd, buf);
    }
    if (bytes == -2) return 0;

    memset(buffer, 0, BUF_SIZE);
    copyArray(buf, buffer, bytes);
    sendRRMessage(fd);
    if (signalMessage == 0)
        signalMessage = 1;
    else
        signalMessage = 0;

    return bytes;
}

void clearBuffer(int fd) {
    unsigned char temp[1] = {0};
    while (read(fd, temp, 1) != 0);
}
