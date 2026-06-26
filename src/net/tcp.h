#ifndef TCP_H_
#define TCP_H_

int half_open_tcp(const char *local_ip, const char *target_ip, unsigned short target_port);
int check_port_open(const char *ip, unsigned short port);

#endif