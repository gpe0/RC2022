// Application layer protocol implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "application_layer.h"

unsigned char* build_control_packet(const char *filename, int flag){
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("File Not Found!\n");
        return NULL;
    }
    // Get the file size
    fseek(fp, 0L, SEEK_END);
    unsigned int file_size = (unsigned int)ftell(fp);
    rewind(fp);

    int CONTROL_PACKET_SIZE = 3 + sizeof(file_size) + 2 + strlen(filename);
    unsigned char* control_packet = (unsigned char *) malloc((CONTROL_PACKET_SIZE*sizeof(char)));
    
    int index = 0;
    control_packet[index++] = flag;

    // TLV - file size
    control_packet[index++] = TLV_FILE_SIZE;
    control_packet[index++] = sizeof(file_size);
    for (int b=0; b<sizeof(file_size); b++){ // build file size by byte
        char byte = ((file_size << (b*8)) & 0xff000000) >> 24;
        // printf("byte: %x\n", ((file_size << (b*8))&0xff000000));
        control_packet[index++] = byte;
    }

    // TLV - file name
    control_packet[index++] = TLV_FILE_NAME;
    control_packet[index++] = strlen(filename);
    for (int c=0; c < strlen(filename); c++){ // build filename by byte
        control_packet[index++] = filename[c];
    }

    for (int i=0; i < CONTROL_PACKET_SIZE; i++){
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

    //free(control_packet);
    printf("Control packet build!\n");
    return control_packet;
}

int build_data_packet(const char *filename, unsigned char* packet){
    /*FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("File Not Found!\n");
        return -1;
    }
    // Get the file size
    fseek(fp, 0L, SEEK_END);
    unsigned int file_size = (unsigned int)ftell(fp);
    rewind(fp);

    char* buffer = (char*) malloc(sizeof(char)*file_size);
    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

    size_t result = fread(buffer, 1, file_size, fp);
    if (result != file_size) {fputs ("Reading error",stderr); exit (3);}

    fclose(fp);

    // recreate copy of the file
    FILE *new_fp;
    new_fp  = fopen("penguin_cp.gif", "w");
    fwrite(buffer , 1 , file_size , new_fp);
    fclose(new_fp);
    
    free(buffer);*/
    return 0;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate, int nTries, int timeout, const char *filename)
{
    printf("role is %s\n", role);
    if (strcmp(role, "tx") == 0){
        //FILE * ptr = fopen(filename, "rb");
        //if (ptr == NULL) return;

        int fd = llopen(serialPort, TRANSMITTER);
        
        if (fd < 0) {
            printf("Error!\n");
            exit(-1);
        }

        unsigned char buffer[100] = "Mensagem Teste ~~~~";

        int bytes = 1;
        int count = 0;
        //bytes = fread(buffer, 1, 100, ptr);
        while( bytes != 0 && count < 100) {
            if (llwrite(fd, buffer, strlen(buffer)) == -1) break;
            //memset(buffer, 0, 100);
            //bytes = fread(buffer, 1, 100, ptr);
            count++;
        }
        printf("here\n");
        //fclose(ptr);
        llclose(fd);
    }
    else if (strcmp(role, "rx") == 0) {
        unsigned char buffer[100] = {0};
        //FILE * ptr = fopen(filename, "ab");
       // if (ptr == NULL) return;

        int fd = llopen(serialPort, RECEIVER);
        
        if (fd < 0) {
            printf("Error!\n");
            exit(-1);
        }
        
        int bytes = 0;
        while(bytes = llread(fd, buffer)) {
            printf("%s %d\n", buffer, bytes);
            //fwrite(buffer, 1, 100, ptr);
        }
       // fclose(ptr);
        llclose(fd);
    }

    /*
        unsigned char* start_packet = build_control_packet(filename, START_PACKET);

        printf("Control packet:\n");
        for (int i=0; i < 20; i++){
                printf("%x ", start_packet[i]);
        }
        printf("\n");

        llwrite(fd, start_packet, sizeof(start_packet));
        printf("Start control packet sent!\n");
        
        free(start_packet);
        llclose(fd);
    }
    if(!strcmp(role, "rx")){
        printf("in receiver\n");
        int fd = llopen(serialPort, RECEIVER);

        if (fd < 0) {
            printf("Error!\n");
            exit(-1);
        }

        unsigned char buf[BUF_SIZE] = {0};
        
        llread(fd, buf);
        printf("Start packet read: %s\n", buf);

        llclose(fd);
    }
    */
}
