#ifndef TCP_HEADER_H_
#define TCP_HEADER_H_

#include <stdint.h>

struct tcp_header {
    uint32_t source_address;
    uint32_t dest_address;
    uint8_t  placeholder;
    uint8_t  protocol;
    uint16_t tcp_length;
};

#endif