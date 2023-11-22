/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "Codec.hpp"
#include "libaccl/BufferPool.hpp"
#include "libaccl/Statistic.hpp"
#include "PacketBuffer.hpp"
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <thread>

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
		// Packet format
		PacketHeaderOptionFormatType packet_format;

		accl::BufferPool<PacketBuffer> *rx_buffer_pool;
		accl::BufferPool<PacketBuffer> *encoder_pool;
		accl::BufferPool<PacketBuffer> *socket_write_pool;

		accl::BufferPool<PacketBuffer> *tx_buffer_pool;
		accl::BufferPool<PacketBuffer> *decoder_pool;
		accl::BufferPool<PacketBuffer> *tap_write_pool;

		// Statistics
		accl::StatisticResult<float> statCompressionRatio;

		int *stop_program;
};
