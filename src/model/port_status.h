#include <stdbool.h>

#ifndef PORT_STATUS_H_
#define PORT_STATUS_H_

typedef struct {
    int port;
    bool open;
} PortStatus;

#endif