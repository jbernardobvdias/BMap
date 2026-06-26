#include "subnet.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// If the user is using windows the logic changes from unix systems
#if defined(_WIN32)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

NetworkInfo get_network_info(void) {
    NetworkInfo info;
    memset(&info, 0, sizeof(info));
    strcpy(info.ip, "0.0.0.0");
    strcpy(info.subnet, "0.0.0.0");

    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
    if (pAdapterInfo == NULL) {
        return info;
    }

    // First call tells us how big the buffer actually needs to be
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            return info;
        }
    }

    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter) {
            const char *ip = pAdapter->IpAddressList.IpAddress.String;
            // Skip adapters that don't have a real address (e.g. disabled NICs)
            if (strcmp(ip, "0.0.0.0") != 0) {
                strncpy(info.ip, ip, sizeof(info.ip) - 1);
                strncpy(info.subnet, pAdapter->IpAddressList.IpMask.String, sizeof(info.subnet) - 1);
                break; // take the first adapter with a valid address
            }
            pAdapter = pAdapter->Next;
        }
    }

    free(pAdapterInfo);
    return info;
}

// If the user is using a unix based system the logic is changes from windows
#elif defined(__linux__) || defined(__APPLE__)

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

#else
#error Unsupported operating system
#endif