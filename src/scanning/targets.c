#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <ifaddrs.h>
#include <errno.h>

#include "targets.h"
#include "net/subnet.h"
#include "net/arp.h"

// Assuming these from your includes
#define BUF_SIZE 60
// ... (keep all your existing structs and defines: arp_header, etc.)

// Simple dynamic list for targets (expand with proper struct)
typedef struct {
    char ip[16];
    unsigned char mac[6];
} Target;

typedef struct {
    Target* targets;
    int count;
    int capacity;
} TargetList;

void init_target_list(TargetList* list) {
    list->count = 0;
    list->capacity = 16;
    list->targets = malloc(list->capacity * sizeof(Target));
}

void add_target(TargetList* list, const char* ip, const unsigned char* mac) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->targets = realloc(list->targets, list->capacity * sizeof(Target));
    }
    strncpy(list->targets[list->count].ip, ip, 15);
    memcpy(list->targets[list->count].mac, mac, 6);
    list->count++;
}

void free_target_list(TargetList* list) {
    free(list->targets);
}

// Main function to implement
TargetList findTargets() {
    TargetList targets;
    init_target_list(&targets);

    // Get subnet info
    NetworkInfo net_info = get_network_info();
    if (strcmp(net_info.ip, "0.0.0.0") == 0) {
        fprintf(stderr, "Failed to get network info\n");
        return targets;
    }

    printf("Interface IP: %s, Subnet: %s\n", net_info.ip, net_info.subnet);

    uint32_t src_ip;
    unsigned char src_mac[6];
    int ifindex;

    if (get_if_info("eth0", &src_ip, src_mac, &ifindex) != 0) {  // Change "eth0" to your interface or make dynamic
        fprintf(stderr, "Failed to get interface info\n");
        return targets;
    }

    int arp_fd = -1;
    if (bind_arp(ifindex, &arp_fd) != 0) {
        fprintf(stderr, "Failed to bind ARP socket\n");
        return targets;
    }

    // Calculate subnet range (simplified - assumes /24 for start; improve with proper mask math)
    struct in_addr net_addr, mask_addr;
    inet_aton(net_info.ip, &net_addr);
    inet_aton(net_info.subnet, &mask_addr);

    uint32_t network = net_addr.s_addr & mask_addr.s_addr;
    uint32_t broadcast = network | ~mask_addr.s_addr;

    printf("Scanning subnet...\n");

    // Loop through possible hosts (skip network/broadcast)
    for (uint32_t i = network + 1; i < broadcast; i++) {
        uint32_t dst_ip = htonl(ntohl(i));  // Proper byte order

        if (send_arp(arp_fd, ifindex, src_mac, src_ip, dst_ip) != 0) {
            continue;
        }

        // Brief listen for reply (non-blocking or timeout in real impl)
        fd_set readfds;
        struct timeval tv = {0, 200000};  // 200ms timeout
        FD_ZERO(&readfds);
        FD_SET(arp_fd, &readfds);

        if (select(arp_fd + 1, &readfds, NULL, NULL, &tv) > 0) {
            // Read reply
            unsigned char buffer[BUF_SIZE];
            ssize_t len = recvfrom(arp_fd, buffer, BUF_SIZE, 0, NULL, NULL);
            if (len > 0) {
                struct arp_header *arp_resp = (struct arp_header *)(buffer + ETH2_HEADER_LEN);
                if (ntohs(arp_resp->opcode) == ARP_REPLY) {
                    struct in_addr sender_ip;
                    memcpy(&sender_ip.s_addr, arp_resp->sender_ip, 4);
                    char ip_str[16];
                    strcpy(ip_str, inet_ntoa(sender_ip));
                    add_target(&targets, ip_str, arp_resp->sender_mac);
                    printf("Found target: %s\n", ip_str);
                }
            }
        }
    }

    close(arp_fd);
    return targets;
}