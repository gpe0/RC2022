// Application layer protocol implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>  
#include "application_layer.h"
#include "link_layer.h"

int sendControlPacket(int fd, const char* filename, int flag){

    unsigned int file_size = getFileSize(filename);
    unsigned int file_size_copy = file_size;
    int nFileSize = 0;
    int mod = 0;

    while (1) {
        mod = file_size % 256;
        file_size /= 256;
        if (file_size == 0 && mod == 0) break;
        nFileSize++;
    }

    unsigned char packet[CONTROL_FIELD_SIZE] = {flag, TLV_FILE_SIZE};

    int index = 2;

    // TLV - file size
    packet[index++] = nFileSize;

    while (1) {
        mod = file_size_copy % 256;
        file_size_copy /= 256;
        if (file_size_copy == 0 && mod == 0) break;
        packet[index++] = mod;
    }


    // TLV - file name
    packet[index++] = TLV_FILE_NAME;
    packet[index++] = strlen(filename);
    for (int c = 0; c < strlen(filename); c++)
    { // build filename by byte
        packet[index++] = filename[c];
    }
    return llwrite(fd, packet, index);
}

int getFileSize(const char *filename){
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("File Not Found!\n");
        return -1;
    }
    // Get the file size
    fseek(fp, 0L, SEEK_END);
    unsigned int file_size = (unsigned int)ftell(fp);
    rewind(fp);
    fclose(fp);
    return file_size;
}



int sendDataPacket(int fd, unsigned char *buffer, unsigned int N, unsigned int length) {
    unsigned char buf[BUF_SIZE] = {DATA_PACKET, (N % 255), length / 256, length % 256};
    for (int i = 0; i < length; i++) {
        buf[4 + i] = buffer[i];
    }

    return llwrite(fd, buf, length + 4);

}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    struct timeval begin, end;
    printf("role is %s\n", role);
    if (strcmp(role, "tx") == 0)
    {
        FILE *ptr = fopen(filename, "rb");
        if (ptr == NULL)
            return;

        LinkLayer linkOptions;
        linkOptions.baudRate = baudRate;
        linkOptions.role = LlTx;
        linkOptions.nRetransmissions = nTries;
        linkOptions.timeout = timeout;
        strcpy(linkOptions.serialPort, serialPort);

        int fd = llopen(linkOptions);

        if (fd < 0)
        {
            printf("Error!\n");
            return;
        }

        sendControlPacket(fd, filename, START_PACKET);
        printf("Start Packet Sent\n");

        unsigned char buffer[DATA_FIELD_SIZE] = {0};


        int bytes = 0;
        int count = 0;
        bytes = fread(buffer, 1, DATA_FIELD_SIZE - DATA_HEADER_SIZE, ptr);

        while (bytes)
        {
            if (sendDataPacket(fd, buffer, count, bytes) == -1) break;
            printf("Packet number %d Sent (%d bytes sent)\n", count, bytes + DATA_HEADER_SIZE);
            memset(buffer, 0, DATA_FIELD_SIZE);
            bytes = fread(buffer, 1, DATA_FIELD_SIZE - DATA_HEADER_SIZE, ptr);
            count++;
        }

        sendControlPacket(fd, filename, END_PACKET);
        printf("End Packet Sent\n");

        llclose(fd);
        fclose(ptr);
    }
    else if (strcmp(role, "rx") == 0)
    {
        FILE *ptr = fopen(filename, "ab");
        if (ptr == NULL)
            return;

        LinkLayer linkOptions;
        linkOptions.baudRate = baudRate;
        linkOptions.role = LlRx;
        linkOptions.nRetransmissions = nTries;
        linkOptions.timeout = timeout;
        strcpy(linkOptions.serialPort, serialPort);

        int fd = llopen(linkOptions);

        if (fd < 0)
        {
            printf("Error!\n");
            return;
        }
        unsigned char buffer[DATA_FIELD_SIZE] = {0};

        int bytes = 0;
        int nExpected = 0;
        bytes = llread(fd, buffer);
        clearBuffer(fd);
        while (bytes)
        {
            unsigned char data[DATA_FIELD_SIZE] = {0};
            for (int i = 0; i < buffer[2] * 256 + buffer[3]; i++) {
                data[i] = buffer[i + 4];
            }
            if (buffer[0] == START_PACKET) {
                printf("\n--START PACKET--\n\n");
                unsigned char filename[CONTROL_FIELD_SIZE] = {0};
                unsigned int filesize = 0;
                int index = 1;
                while (buffer[index] != '\0') {
                    if (buffer[index] == TLV_FILE_NAME) {
                        index++;
                        int lim = buffer[index];      
                        for (int i = 0; i < lim; i++) {
                            filename[i] = buffer[++index];
                        }
                        index++;
                    }
                    else if (buffer[index] == TLV_FILE_SIZE) {
                        index++;
                        int lim = buffer[index];
                        for (int i = 0; i < lim; i++) {
                            int multiplier = 1;
                            for (int j = 0; j < i; j++) {
                                multiplier *= 256; 
                            }
                            filesize += multiplier * buffer[++index];
                        }
                        index++;
                    }
                }
                printf("filename: %s\n", filename);
                printf("filesize: %d bytes\n\n", filesize);
                gettimeofday(&begin, NULL);
            }
            else if (buffer[0] == END_PACKET) {
                printf("\n--END PACKET--\n\n");
                unsigned char filename[CONTROL_FIELD_SIZE] = {0};
                unsigned int filesize = 0;
                int index = 1;
                while (buffer[index] != '\0') {
                    if (buffer[index] == TLV_FILE_NAME) {
                        index++;
                        int lim = buffer[index];      
                        for (int i = 0; i < lim; i++) {
                            filename[i] = buffer[++index];
                        }
                        index++;
                    }
                    else if (buffer[index] == TLV_FILE_SIZE) {
                        index++;
                        int lim = buffer[index];
                        for (int i = 0; i < lim; i++) {
                            int multiplier = 1;
                            for (int j = 0; j < i; j++) {
                                multiplier *= 256; 
                            }
                            filesize += multiplier * buffer[++index];
                        }
                        index++;
                    }
                }
                printf("filename: %s\n", filename);
                printf("filesize: %d bytes\n", filesize);
                gettimeofday(&end, NULL);
                printf("Time Spent : %f Seconds\n\n",(double) ((((end.tv_sec - begin.tv_sec) * 1000000) + end.tv_usec) - (begin.tv_usec)) / 1000000);
            }
            else if (buffer[0] == DATA_PACKET) {
                if (buffer[1] != nExpected++) return;

                fwrite(data, 1, buffer[2] * 256 + buffer[3], ptr);
                printf("Wrote to GIF file - Packet number %d (%d bytes received)\n", buffer[1], bytes);
            }
            bytes = llread(fd, buffer);
        }
        fclose(ptr);
        llclose(fd);
    }
}
