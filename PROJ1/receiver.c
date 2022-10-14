#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "api.h"
#include "application_layer.h"

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

    FILE * ptr = fopen("test.gif", "ab");
    if (ptr == NULL) return 1;

    int fd = llopen(serialPortName, RECEIVER);

    if (fd < 0) {
        printf("Error!\n");
        return -1;
    }
    unsigned char buf[BUF_SIZE] = {0};

    int bytes = 0;
    int count = 1;
    while (bytes = llread(fd, buf)) {
    fwrite(buf, 1, 100, ptr);
    printf("Wrote to GIF file - %d\n", count++);
   }

    /*

    for (int i = 0; i < 100; i++) {
        llread(fd, buf);
        printf("String read: %s\n", buf);
    }

    */

    llclose(fd);

    // testing application layer
    //printf("Testing application layer...\n");
    
   // applicationLayer(serialPortName, "rx", 0,0,0, "test.gif");
    return 0;
}
