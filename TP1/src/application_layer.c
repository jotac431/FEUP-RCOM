// Application layer protocol implementation

#include "application_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer layer;

    layer.baudRate = baudRate;
    layer.nRetransmissions = nTries;

    switch (*role)
    {
    case 't':
        layer.role = LlTx;
        break;
    case 'r':
        layer.role = LlRx;
        break;
    default:
        break;
    }

    sprintf(layer.serialPort, "%s", *serialPort);
    layer.timeout = timeout;

    llopen(layer);
}
