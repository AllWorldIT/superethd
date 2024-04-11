/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#if 0
#pragma once

#include "libaccl/buffer_pool.hpp"
#include "libaccl/statistic.hpp"
#include "packet_buffer.hpp"
#include "packet_switch.hpp"
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string>

struct TAPInterface {
		int fd;
		unsigned char hwaddr[ETHER_ADDR_LEN];
		std::string ifname;
};

/*
// Structure to hold data for the threads
struct ThreadData {
		// TAP device
		struct TAPInterface tap_interface;
		// UDP socket
		int udp_socket;
		// Settings
		uint16_t tx_size;
		uint16_t mtu;

		// Local and remote address
		struct sockaddr_in6 local_addr;
		struct sockaddr_in6 remote_addr;

		// Packet switch
		PacketSwitch *packet_switch;

		// Statistics
		accl::StatisticResult<float> statCompressionRatio;

		int *stop_program;
};
*/
#endif