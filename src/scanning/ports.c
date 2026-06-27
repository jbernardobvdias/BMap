#include "ports.h"

#include <stdio.h>

PortStatus findPorts(char* ip, int* portsList) {
    
    int ipSize = sizeof(portsList) / sizeof(portsList[0]);
    int portsListSize = sizeof(portsList) / sizeof(portsList[0]);

    PortStatus status[portsListSize] = {};

    for (int i = 0; i < ipSize; i++) {
        for (int j = 0; j < portsListSize; j++) {
            printf("");
        }
    }

    return *status;
}