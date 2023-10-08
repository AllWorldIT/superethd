#ifndef SET_H
#define SET_H

#include <net/if.h>

extern int start_set(char *ifname, struct in6_addr *src, struct in6_addr *dst, int port, int mtu, int mss);

#endif