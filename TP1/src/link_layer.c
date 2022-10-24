// Link layer protocol implementation

#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

struct termios oldtio;
struct termios newtio;

int alarmEnabled = FALSE;
int alarmCount = 0;

volatile int STOP = FALSE;

int state = START;

void stateMachine(int fd, unsigned char c)
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
            if (byte == c)
                state = C_RCV;
            else if (byte == FLAG)
                state = FLAG_RCV;
            else
                state = START;
            break;
        case C_RCV:
            printf("C_RCV\n");
            if (byte == (A ^ c))
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
                if (c == C_UA) alarm(0);
            }
            else
                state = START;
            break;
        }
    }
}

int sendBuffer(int fd, unsigned char c)
{

    // Create string to send
    unsigned char buf[5];

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = c;
    buf[3] = A ^ c;
    buf[4] = FLAG;

    return write(fd, buf, sizeof(buf));
}

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{

    (void)signal(SIGALRM, alarmHandler);
    
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
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

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
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

    int bytes = 0;

    // Wait until all bytes have been written to the serial port
    sleep(1);

    STOP = FALSE;

    switch (connectionParameters.role)
    {
    case LlTx:

        while (STOP == FALSE && alarmCount < 4)
        {
            if (alarmEnabled == FALSE)
            {
                printf("eheh\n");
                bytes = sendBuffer(fd, C);
                printf("%d bytes written\n", bytes);
                alarm(3); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;
            }

            stateMachine(fd, C_UA);
        }
        break;
    case LlRx:
        while (STOP == FALSE)
        {
            stateMachine(fd, C);
        }
        bytes = sendBuffer(fd, C_UA);
        printf("%d bytes written\n", bytes);
        break;
    default:
        break;
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

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
