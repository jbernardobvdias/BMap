#include "tcp.h"

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Pseudo header required by the TCP checksum algorithm (RFC 793)
struct pseudo_header {
    uint32_t source_address;
    uint32_t dest_address;
    uint8_t  placeholder;
    uint8_t  protocol;
    uint16_t tcp_length;
};

static unsigned short checksum(unsigned short *ptr, int nbytes) {
    long sum = 0;
    unsigned short oddbyte;
    short answer;

    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }
    if (nbytes == 1) {
        oddbyte = 0;
        *((unsigned char *)&oddbyte) = *(unsigned char *)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = (short)~sum;
    return answer;
}

// Sends a single SYN packet and deliberately does NOT complete the handshake (no ACK is ever sent).
// Requires root or CAP_NET_RAW, since raw sockets bypass the normal connection-tracking the kernel does for you.
int half_open_tcp(const char *local_ip, const char *target_ip, unsigned short target_port) {
    int sockfd;
    char packet[4096];
    struct iphdr *iph = (struct iphdr *)packet;
    struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));
    struct sockaddr_in dest;
    struct pseudo_header psh;
    char pseudogram[sizeof(struct pseudo_header) + sizeof(struct tcphdr)];
    unsigned short source_port = 54321; /* arbitrary ephemeral-style port */

    memset(packet, 0, sizeof(packet));

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("socket() failed (need root/CAP_NET_RAW)");
        return -1;
    }

    dest.sin_family = AF_INET;
    dest.sin_port = htons(target_port);
    dest.sin_addr.s_addr = inet_addr(target_ip);

    // IP header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
    iph->id = htons(54321);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0; /* kernel fills this in because of IP_HDRINCL below */
    iph->saddr = inet_addr(local_ip);
    iph->daddr = dest.sin_addr.s_addr;

    // TCP header
    tcph->source = htons(source_port);
    tcph->dest = htons(target_port);
    tcph->seq = htonl(0);
    tcph->ack_seq = 0;
    tcph->doff = 5; /* header length in 32-bit words, no options */
    tcph->syn = 1;  /* SYN only — this is the "half" part of half-open */
    tcph->window = htons(5840);
    tcph->check = 0;
    tcph->urg_ptr = 0;

    // TCP checksum, computed over a pseudo header + the TCP segment
    psh.source_address = iph->saddr;
    psh.dest_address = iph->daddr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr));

    memcpy(pseudogram, &psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), tcph, sizeof(struct tcphdr));
    tcph->check = checksum((unsigned short *)pseudogram, sizeof(pseudogram));

    // Tell the kernel "I've already built the IP header myself"
    int one = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt(IP_HDRINCL) failed");
        close(sockfd);
        return -1;
    }

    if (sendto(sockfd, packet, iph->tot_len, 0,
               (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        perror("sendto failed");
        close(sockfd);
        return -1;
    }

    printf("SYN sent to %s:%u from port %u — handshake intentionally left "
           "incomplete (no ACK sent)\n", target_ip, target_port, source_port);

    close(sockfd);
    return 0;
}

// Check if port is open with full handshake
int full_tcp(const char *ip, unsigned short port) {
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(fd, F_SETFL, O_NONBLOCK);

    connect(fd, (struct sockaddr *)&addr, sizeof(addr));

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);
    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 }; // 1s timeout

    int open = 0;
    if (select(fd + 1, NULL, &wfds, NULL, &tv) > 0) {
        int err; socklen_t len = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
        open = (err == 0); // 0 = connected = port open
    }
    close(fd);
    return open;
}