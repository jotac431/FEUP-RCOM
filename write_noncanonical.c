// Write to serial port in non-canonical mode
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
#include <signal.h>

struct termios oldtio;
struct termios newtio;

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

// SET buffer values
#define FLAG 0x7E
#define A 0x03
#define C 0x03
#define C_UA 0x07
#define BCC (A ^ C)

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define S_STOP 5

int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

volatile int STOP = FALSE;

int state = START;

void stateMachine(int fd, unsigned char a)
{

    unsigned char byte = 0;

    int bytes = 0;

    bytes = read(fd, &byte, 1);

    if (bytes > 0)
    {
        printf("%x\n", byte);
        switch (state)
        {
        case START:
            printf("START\n");
            if (byte == FLAG)
                state = FLAG_RCV;
            break;
        case FLAG_RCV:
            printf("FLAG_RCV\n");
            if (byte == A)
                state = A_RCV;
            else if (byte != FLAG)
                state = START;
            break;
        case A_RCV:
            printf("A_RCV\n");
            if (byte == a)
                state = C_RCV;
            else if (byte == FLAG)
                state = FLAG_RCV;
            else
                state = START;
            break;
        case C_RCV:
            printf("C_RCV\n");
            if (byte == BCC)
                state = BCC_OK;
            else if (byte == FLAG)
                state = FLAG_RCV;
            else
                state = START;
            break;
        case BCC_OK:
            printf("BCC_OK\n");
            if (byte == FLAG)
            {
                printf("STOP\n");
                STOP = TRUE;
                state = START;
                alarm(0);
            }
            else
                state = START;
            break;
        }
    }
}

int sendBuffer(int fd, unsigned char a){

    // Create string to send
    unsigned char buf[5];

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC;
    buf[4] = FLAG;

    return write(fd, buf, sizeof(buf));
}

int ll_open(int fd){

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

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
        exit(-1);
    }

    printf("New termios structure set\n");

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.

    // Set alarm function handler

    int bytes = sendBuffer(fd, C);

    printf("%d bytes written\n", bytes);

    // Wait until all bytes have been written to the serial port
    sleep(1);

    bytes = 0;

    while (STOP == FALSE && alarmCount < 4)
    {
        if (alarmEnabled == FALSE)
        {
            bytes = sendBuffer(fd, C);
            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }

        stateMachine(fd, C_UA);
        
    }

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}

int main(int argc, char *argv[])
{

    // Program usage: Uses either COM1 or COM2
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

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    (void)signal(SIGALRM, alarmHandler);

    if (!ll_open(fd))
        return -1;
}
