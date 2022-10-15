// Application layer protocol implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "application_layer.h"

unsigned char *build_control_packet(const char *filename, int flag)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("File Not Found!\n");
        return NULL;
    }
    // Get the file size
    fseek(fp, 0L, SEEK_END);
    unsigned int file_size = (unsigned int)ftell(fp);
    rewind(fp);

    int CONTROL_PACKET_SIZE = 3 + sizeof(file_size) + 2 + strlen(filename);
    unsigned char *control_packet = (unsigned char *)malloc((CONTROL_PACKET_SIZE * sizeof(char)));

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

    for (int i = 0; i < CONTROL_PACKET_SIZE; i++)
    {
        printf("%x ", control_packet[i]);
    }
    printf("\n");
    /*
    printf("Image data:\n");
    for (int i=0; i < 20; i++){
        printf("%x ", buffer[i]);
    }
    printf("\n");
    */

    // free(control_packet);
    printf("Control packet build!\n");
    return control_packet;
}

int applicationLayer(const char *serialPort, const char *role, int baudRate, int nTries, int timeout, const char *filename)
{
    printf("role is %s\n", role);
    if (strcmp(role, "tx") == 0)
    {
        FILE *ptr = fopen(filename, "rb");
        if (ptr == NULL)
            return 1;

        int fd = llopen(serialPort, TRANSMITTER);

        if (fd < 0)
        {
            printf("Error!\n");
            return 1;
        }

        unsigned char buffer[100] = {0};

        int bytes = 0;
        bytes = fread(buffer, 1, 100, ptr);

        while (bytes)
        {
            llwrite(fd, buffer, bytes);
            memset(buffer, 0, 100);
            bytes = fread(buffer, 1, 100, ptr);
            printf("bytes - %d\n", bytes);
        }

        llclose(fd);
        fclose(ptr);
    }
    else if (strcmp(role, "rx") == 0)
    {
        FILE *ptr = fopen(filename, "ab");
        if (ptr == NULL)
            return 1;

        int fd = llopen(serialPort, RECEIVER);

        if (fd < 0)
        {
            printf("Error!\n");
            return 1;
        }
        unsigned char buffer[BUF_SIZE] = {0};

        int bytes = 0;
        int count = 1;
        while (bytes = llread(fd, buffer))
        {
            fwrite(buffer, 1, bytes, ptr);
            printf("Wrote to GIF file - %d\n", count++);
        }
        fclose(ptr);
        llclose(fd);
    }
}
