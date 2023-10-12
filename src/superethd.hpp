#pragma once

extern "C" {
#include <netinet/in.h>
}

#include <string>

int start_set(const std::string ifname, struct in6_addr *src, struct in6_addr *dst, int port, int mtu, int tx_size);
