#include "subnet.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ifaddrs.h>

NetworkInfo get_network_info(void) {
    NetworkInfo info;
    memset(&info, 0, sizeof(info));
    strcpy(info.ip, "0.0.0.0");
    strcpy(info.subnet, "0.0.0.0");

    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return info;
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) {
            continue; // only care about IPv4 here
        }
        if (strcmp(ifa->ifa_name, "lo") == 0) {
            continue; // skip loopback
        }

        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
        inet_ntop(AF_INET, &addr->sin_addr, info.ip, sizeof(info.ip));

        if (ifa->ifa_netmask != NULL) {
            struct sockaddr_in *mask = (struct sockaddr_in *)ifa->ifa_netmask;
            inet_ntop(AF_INET, &mask->sin_addr, info.subnet, sizeof(info.subnet));
        }
        break; // take the first valid non-loopback interface
    }

    freeifaddrs(ifaddr);
    return info;
}