#ifndef UTIL_H
#define UTIL_H

#include <netinet/in.h>

extern int to_sin6addr(const char *address_str, struct in6_addr *result);

static inline int is_ipv4_mapped_ipv6(const struct in6_addr *ipv6Addr) {
	const uint16_t *addrBlocks = (const uint16_t *)(ipv6Addr->s6_addr);

	// Check the pattern for IPv4-mapped IPv6 addresses
	return (addrBlocks[0] == 0x0000 && addrBlocks[1] == 0x0000 && addrBlocks[2] == 0x0000 && addrBlocks[3] == 0x0000 &&
			addrBlocks[4] == 0x0000 && addrBlocks[5] == 0xffff);
}

#endif