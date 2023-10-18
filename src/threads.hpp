/*
 * Threads handling.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

extern "C" {
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
}

#include "buffers.hpp"

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
	uint16_t max_payload_size;
	uint16_t max_ethernet_frame_size;
	// Local and remote address
	struct sockaddr_in6 local_addr;
	struct sockaddr_in6 remote_addr;
	// Packet buffers
	BufferList available_buffers;
	BufferList tx_packet_queue;
	BufferList rx_packet_queue;

	int *stop_program;
};
