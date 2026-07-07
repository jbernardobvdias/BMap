#ifndef PORT_STATUS_H_
#define PORT_STATUS_H_

#include <stdbool.h>

typedef struct {
    int port;
    bool open;
} PortStatus;

#endif