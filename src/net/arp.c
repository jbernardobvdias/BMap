#include "arp.h"

#include "model/arp_header.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>  // htons etc

#define PROTO_ARP 0x0806
#define ETH2_HEADER_LEN 14
#define HW_TYPE 1
#define MAC_LENGTH 6
#define IPV4_LENGTH 4
#define ARP_REQUEST 0x01
#define ARP_REPLY 0x02
#define BUF_SIZE 60

// Converts struct sockaddr with an IPv4 address to network byte order uint32_t.
// Returns 0 on success.
int int_ip4(struct sockaddr *addr, uint32_t *ip) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *i = (struct sockaddr_in *) addr;
        *ip = i->sin_addr.s_addr;
        return 0;
    } else {
        fprintf(stderr, "Not AF_INET\n");
        return 1;
    }
}

// Formats sockaddr containing IPv4 address as human readable string.
int format_ip4(struct sockaddr *addr, char *out) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *i = (struct sockaddr_in *) addr;
        const char *ip = inet_ntoa(i->sin_addr);
        if (!ip) {
            return -2;
        } else {
            strcpy(out, ip);
            return 0;
        }
    } else {
        return -1;
    }
}

// Writes interface IPv4 address as network byte order to ip.
int get_if_ip4(int fd, const char *ifname, uint32_t *ip) {
    int ret = -1;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));

    if (strlen(ifname) > (IFNAMSIZ - 1)) {
        fprintf(stderr, "Too long interface name\n");
        goto out;
    }

    strcpy(ifr.ifr_name, ifname);
    if (ioctl(fd, SIOCGIFADDR, &ifr) == -1) {
        perror("SIOCGIFADDR");
        goto out;
    }

    if (int_ip4(&ifr.ifr_addr, ip)) {
        goto out;
    }
    ret = 0;
out:
    return ret;
}

// Sends an ARP who-has request to dst_ip on interface ifindex, using source mac src_mac and source ip src_ip.
int send_arp(int fd, int ifindex, const unsigned char *src_mac, uint32_t src_ip, uint32_t dst_ip) {
    int ret = -1;
    unsigned char buffer[BUF_SIZE];
    memset(buffer, 0, sizeof(buffer));

    struct sockaddr_ll socket_address;
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_ARP);
    socket_address.sll_ifindex = ifindex;
    socket_address.sll_hatype = htons(ARPHRD_ETHER);
    socket_address.sll_pkttype = PACKET_BROADCAST;
    socket_address.sll_halen = MAC_LENGTH;

    struct ethhdr *send_req = (struct ethhdr *) buffer;
    struct arp_header *arp_req = (struct arp_header *) (buffer + ETH2_HEADER_LEN);

    // Broadcast destination
    memset(send_req->h_dest, 0xff, MAC_LENGTH);

    // Target MAC unknown (that's what we're asking for)
    memset(arp_req->target_mac, 0x00, MAC_LENGTH);

    // Source MAC = our own MAC
    memcpy(send_req->h_source, src_mac, MAC_LENGTH);
    memcpy(arp_req->sender_mac, src_mac, MAC_LENGTH);
    memcpy(socket_address.sll_addr, src_mac, MAC_LENGTH);

    // Ethertype = ARP
    send_req->h_proto = htons(ETH_P_ARP);

    // Fill in ARP header fields
    arp_req->hardware_type = htons(HW_TYPE);
    arp_req->protocol_type = htons(ETH_P_IP);
    arp_req->hardware_len = MAC_LENGTH;
    arp_req->protocol_len = IPV4_LENGTH;
    arp_req->opcode = htons(ARP_REQUEST);

    memcpy(arp_req->sender_ip, &src_ip, sizeof(uint32_t));
    memcpy(arp_req->target_ip, &dst_ip, sizeof(uint32_t));

    ssize_t sent = sendto(fd, buffer, ETH2_HEADER_LEN + sizeof(struct arp_header), 0,
                           (struct sockaddr *) &socket_address, sizeof(socket_address));
    if (sent == -1) {
        perror("sendto()");
        goto out;
    }
    ret = 0;
out:
    return ret;
}

// Gets interface information by name: IPv4, MAC, ifindex.
int get_if_info(const char *ifname, uint32_t *ip, unsigned char *mac, int *ifindex) {
    int ret = -1;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    int sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sd < 0) {
        perror("socket()");
        goto out;
    }

    if (strlen(ifname) > (IFNAMSIZ - 1)) {
        fprintf(stderr, "Too long interface name, MAX=%i\n", IFNAMSIZ - 1);
        goto out;
    }
    strcpy(ifr.ifr_name, ifname);

    // Get interface index
    if (ioctl(sd, SIOCGIFINDEX, &ifr) == -1) {
        perror("SIOCGIFINDEX");
        goto out;
    }
    *ifindex = ifr.ifr_ifindex;

    // Get MAC address
    if (ioctl(sd, SIOCGIFHWADDR, &ifr) == -1) {
        perror("SIOCGIFHWADDR");
        goto out;
    }
    memcpy(mac, ifr.ifr_hwaddr.sa_data, MAC_LENGTH);

    // Get IPv4 address
    if (get_if_ip4(sd, ifname, ip)) {
        goto out;
    }

    ret = 0;
out:
    if (sd >= 0) {
        close(sd);
    }
    return ret;
}

// Creates a raw socket that listens for ARP traffic on specific ifindex. Writes out the socket's FD.
int bind_arp(int ifindex, int *fd) {
    int ret = -1;

    *fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (*fd < 0) {
        perror("socket()");
        goto out;
    }

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(struct sockaddr_ll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifindex;
    if (bind(*fd, (struct sockaddr *) &sll, sizeof(struct sockaddr_ll)) < 0) {
        perror("bind");
        goto out;
    }

    ret = 0;
out:
    if (ret && *fd >= 0) {
        close(*fd);
    }
    return ret;
}

// Reads a single ARP reply from fd. Returns 0 if a valid reply was read.
int read_arp(int fd) {
    int ret = -1;
    unsigned char buffer[BUF_SIZE];
    ssize_t length = recvfrom(fd, buffer, BUF_SIZE, 0, NULL, NULL);
    if (length == -1) {
        perror("recvfrom()");
        goto out;
    }

    struct ethhdr *rcv_resp = (struct ethhdr *) buffer;
    struct arp_header *arp_resp = (struct arp_header *) (buffer + ETH2_HEADER_LEN);

    if (ntohs(rcv_resp->h_proto) != PROTO_ARP) {
        goto out; // not an ARP packet
    }
    if (ntohs(arp_resp->opcode) != ARP_REPLY) {
        goto out; // not a reply (e.g. someone else's request)
    }

    struct in_addr sender_a;
    memset(&sender_a, 0, sizeof(sender_a));
    memcpy(&sender_a.s_addr, arp_resp->sender_ip, sizeof(uint32_t));

    printf("Sender IP: %s\n", inet_ntoa(sender_a));
    printf("Sender MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           arp_resp->sender_mac[0], arp_resp->sender_mac[1], arp_resp->sender_mac[2],
           arp_resp->sender_mac[3], arp_resp->sender_mac[4], arp_resp->sender_mac[5]);

    ret = 0;
out:
    return ret;
}

// Sample code that sends an ARP who-has request on interface <ifname> to IPv4 address <ip>.
int test_arping(const char *ifname, const char *ip) {
    int ret = -1;
    int arp_fd = -1;

    uint32_t dst_ip = inet_addr(ip);
    if (dst_ip == 0 || dst_ip == 0xffffffff) {
        fprintf(stderr, "Invalid destination IP\n");
        return 1;
    }

    uint32_t src_ip;
    int ifindex;
    unsigned char mac[MAC_LENGTH];

    if (get_if_info(ifname, &src_ip, mac, &ifindex)) {
        fprintf(stderr, "get_if_info failed, interface %s not found or no IP set?\n", ifname);
        goto out;
    }

    if (bind_arp(ifindex, &arp_fd)) {
        fprintf(stderr, "Failed to bind_arp()\n");
        goto out;
    }

    if (send_arp(arp_fd, ifindex, mac, src_ip, dst_ip)) {
        fprintf(stderr, "Failed to send_arp\n");
        goto out;
    }

    while (1) {
        if (read_arp(arp_fd) == 0) {
            printf("Got reply, breaking out\n");
            break;
        }
    }

    ret = 0;
out:
    if (arp_fd >= 0) {
        close(arp_fd);
    }
    return ret;
}