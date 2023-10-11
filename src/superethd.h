#ifndef __SUPERETHD_H__
#define __SUPERETHD_H__

#include <netinet/in.h>

extern int start_set(char *ifname, struct in6_addr *src, struct in6_addr *dst, int port, int mtu, int tx_size);

#endif