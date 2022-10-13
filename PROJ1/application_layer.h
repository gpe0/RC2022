// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#define DATA_PACKET 1
#define START_PACKET 2
#define END_PACKET 3

#define TLV_FILE_SIZE 0
#define TLV_FILE_NAME 1

#include "api.h"

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

unsigned char* build_control_packet(const char *filename, int flag);

int build_data_packet(const char *filename, unsigned char* packet);

#endif // _APPLICATION_LAYER_H_
