// Read from serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "api.h"


int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    volatile int STOP = FALSE;

    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    int fd = llopen(serialPortName, RECEIVER);

    if (fd < 0) {
        printf("Error!\n");
        return -1;
    }

    llclose(fd);

    return 0;
}
