/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "libaccl/BufferPool.hpp"
#include <thread>

#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>

struct TAPInterface {
		int fd;
		unsigned char hwaddr[ETHER_ADDR_LEN];
		char ifname[IF_NAMESIZE + 1];
};

// Structure to hold data for the threads
struct ThreadData {
		// TAP device
		struct TAPInterface tap_device;
		// UDP socket
		int udp_socket;
		// Settings
		uint16_t tx_size;
		uint16_t mtu;
		uint16_t l4mtu;
		uint16_t l2mtu;
		// Local and remote address
		struct sockaddr_in6 local_addr;
		struct sockaddr_in6 remote_addr;

		int *stop_program;
};
