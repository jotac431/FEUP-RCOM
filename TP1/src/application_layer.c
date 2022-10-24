// Application layer protocol implementation

#include "application_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer layer;

    layer.baudRate = baudRate;
    layer.nRetransmissions = nTries;

    if (strcmp(role, "tx") == 0) layer.role = LlTx;
    if (strcmp(role, "rx") == 0) layer.role = LlRx;

    sprintf(layer.serialPort, "%s", serialPort);
    layer.timeout = timeout;

    llopen(layer);

}
