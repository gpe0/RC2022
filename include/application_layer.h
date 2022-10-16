// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#define DATA_FIELD_SIZE 100
#define DATA_PACKET 1
#define START_PACKET 2
#define END_PACKET 3

#define TLV_FILE_SIZE 0
#define TLV_FILE_NAME 1

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

int sendDataPacket(int fd, unsigned char *buffer, unsigned int N, unsigned int length);

int getControlPacketSize(const char *filename);

int buildControlPacket(const char *filename, int flag, unsigned char* packet, int packet_size);

unsigned int getFileSize(const char *filename);

int sendControlPacket(int fd, const char* filename, int flag);

#endif // _APPLICATION_LAYER_H_
