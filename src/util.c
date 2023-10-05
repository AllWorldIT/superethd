

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int to_sin6addr(const char *address_str, struct in6_addr *result) {
	// Try converting as IPv6 address
	if (inet_pton(AF_INET6, address_str, result) > 0) {
		return 1;  // Successfully converted as IPv6
	}

	// Try converting as IPv4 address and map it to IPv6
	struct in_addr ipv4_addr;
	if (inet_pton(AF_INET, address_str, &ipv4_addr) > 0) {
		// Convert IPv4 address to IPv6-mapped IPv6 address
		memset(result, 0, sizeof(struct in6_addr));
		result->s6_addr[10] = 0xFF;
		result->s6_addr[11] = 0xFF;
		memcpy(&result->s6_addr[12], &ipv4_addr.s_addr, sizeof(ipv4_addr.s_addr));
		return 1;  // Successfully converted as IPv4-mapped IPv6
	}

	// Conversion failed
	return 0;
}


