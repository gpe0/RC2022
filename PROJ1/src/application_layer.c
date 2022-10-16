// Application layer protocol implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "application_layer.h"
#include "link_layer.h"

int sendControlPacket(int fd, const char* filename, int flag){
    const int packet_size = getControlPacketSize(filename);
    if (packet_size < 0){
        printf("Error data packet size!\n");
        return 1;
    }

    unsigned char packet[packet_size];
    buildControlPacket(filename, START_PACKET, packet, packet_size);
    llwrite(fd, packet, packet_size);
}

unsigned int getFileSize(const char *filename){
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("File Not Found!\n");
        return NULL;
        return -1;
    }
    // Get the file size
    fseek(fp, 0L, SEEK_END);
    unsigned int file_size = (unsigned int)ftell(fp);
    rewind(fp);
    fclose(fp);
    return file_size;
}

int getControlPacketSize(const char *filename){
    unsigned int file_size = getFileSize(filename);
    int CONTROL_PACKET_SIZE = 3 + sizeof(file_size) + 2 + strlen(filename);
    unsigned char *control_packet = (unsigned char *)malloc((CONTROL_PACKET_SIZE * sizeof(char)));
    return CONTROL_PACKET_SIZE;
}

int buildControlPacket(const char *filename, int flag, unsigned char* control_packet, int packet_size)
{
    unsigned int file_size = getFileSize(filename);
    int index = 0;
    control_packet[index++] = flag;

    // TLV - file size
    control_packet[index++] = TLV_FILE_SIZE;
    control_packet[index++] = sizeof(file_size);
    for (int b = 0; b < sizeof(file_size); b++)
    { // build file size by byte
        char byte = ((file_size << (b * 8)) & 0xff000000) >> 24;
        // printf("byte: %x\n", ((file_size << (b*8))&0xff000000));
        control_packet[index++] = byte;
    }

    // TLV - file name
    control_packet[index++] = TLV_FILE_NAME;
    control_packet[index++] = strlen(filename);
    for (int c = 0; c < strlen(filename); c++)
    { // build filename by byte
        control_packet[index++] = filename[c];
    }

    printf("Control packet build!\n");
    return 0;
}

int sendDataPacket(int fd, unsigned char *buffer, unsigned int N, unsigned int length) {
    unsigned char buf[BUF_SIZE] = {DATA_PACKET, (N % 255), 0, length};

    for (int i = 0; i < length; i++) {
        buf[4 + i] = buffer[i];
    }

    return llwrite(fd, buf, length + 4);

}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    printf("role is %s\n", role);
    if (strcmp(role, "tx") == 0)
    {
        FILE *ptr = fopen(filename, "rb");
        if (ptr == NULL)
            return;

        int fd = llopen(serialPort, TRANSMITTER);

        if (fd < 0)
        {
            printf("Error!\n");
            return;
        }

        sendControlPacket(fd, filename, START_PACKET);

        unsigned char buffer[DATA_FIELD_SIZE] = {0};

        int bytes = 0;
        int count = 0;
        bytes = fread(buffer, 1, DATA_FIELD_SIZE, ptr);

        while (bytes)
        {
            if (sendDataPacket(fd, buffer, count, bytes) == -1) break;
            memset(buffer, 0, DATA_FIELD_SIZE);
            bytes = fread(buffer, 1, DATA_FIELD_SIZE, ptr);
            printf("Packet nmr %d\n", count);
            count++;
        }

        sendControlPacket(fd, filename, END_PACKET);

        llclose(fd);
        fclose(ptr);
    }
    else if (strcmp(role, "rx") == 0)
    {
        FILE *ptr = fopen(filename, "ab");
        if (ptr == NULL)
            return;

        int fd = llopen(serialPort, RECEIVER);

        if (fd < 0)
        {
            printf("Error!\n");
            return;
        }
        unsigned char buffer[BUF_SIZE] = {0};

        int bytes = 0;
        int nExpected = 0;
        while (bytes = llread(fd, buffer))
        {
            unsigned char data[BUF_SIZE] = {0};
            for (int i = 0; i < buffer[2] * 256 + buffer[3]; i++) {
                data[i] = buffer[i + 4];
            }
            if (buffer[0] == START_PACKET) {
                printf("START PACKET\n");
            }
            else if (buffer[0] == END_PACKET) {
                printf("END PACKET\n");
            }
            else if (buffer[0] == DATA_PACKET) {
                if (buffer[1] != nExpected++) return;

                fwrite(data, 1, buffer[2] * 256 + buffer[3], ptr);
                printf("Wrote to GIF file\n");
            }
            
        }
        fclose(ptr);
        llclose(fd);
    }
}
