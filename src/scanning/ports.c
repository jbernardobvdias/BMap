#include "ports.h"
#include "net/subnet.h"
#include "net/tcp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PortStatus* findPorts(const char* target_ip, int* portsList, int numPorts) {
    if (!target_ip || !portsList || numPorts <= 0) {
        fprintf(stderr, "Invalid arguments to findPorts\n");
        return NULL;
    }

    PortStatus* status = calloc(numPorts, sizeof(PortStatus));
    if (!status) {
        perror("calloc failed");
        return NULL;
    }

    // Get local IP for raw socket scans
    NetworkInfo net_info = get_network_info();
    if (strcmp(net_info.ip, "0.0.0.0") == 0) {
        fprintf(stderr, "Failed to get local IP\n");
        free(status);
        return NULL;
    }

    printf("Scanning %d ports on %s...\n", numPorts, target_ip);

    for (int i = 0; i < numPorts; i++) {
        int port = portsList[i];
        if (port < 1 || port > 65535) {
            status[i] = PORT_ERROR;
            continue;
        }

        int open = full_tcp(target_ip, (unsigned short)port);
        
        if (open) {
            status[i] = PORT_OPEN;
            printf("Port %d OPEN\n", port);
        } else {
            // Fallback to half-open SYN scan if full fails (requires root)
            int syn_result = half_open_tcp(net_info.ip, target_ip, (unsigned short)port);
            if (syn_result == 0) {
                // In real impl we would listen for SYN-ACK here.
                // For now we fall back to full scan result.
                status[i] = PORT_CLOSED;  // TODO: improve with reply listening
            } else {
                status[i] = PORT_FILTERED; // or error
            }
            printf("Port %d CLOSED or FILTERED\n", port);
        }
    }

    return status;
}