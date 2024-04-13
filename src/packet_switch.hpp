/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "codec.hpp"
#include "fdb.hpp"
#include "libaccl/buffer_pool.hpp"
#include "packet_buffer.hpp"
#include "remote_node.hpp"
#include "tap_interface.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <sys/types.h>

class PacketSwitch {
	public:
		PacketSwitch(const std::string ifname, int mtu, int tx_size, PacketHeaderOptionFormatType packet_format,
					 std::shared_ptr<struct sockaddr_storage> src_addr,
					 std::vector<std::shared_ptr<struct sockaddr_storage>> dst_addrs, int port);
		~PacketSwitch();

		void start();
		void wait();
		void stop();

		inline uint16_t getMTUSize() { return this->mtu; }

		inline uint16_t getL2MTUSize() { return this->l2mtu; }

		inline PacketHeaderOptionFormatType getPacketFormat() { return this->packet_format; }

	private:
		// FDB and mutex protecting it
		std::shared_ptr<FDB> fdb;
		mutable std::shared_mutex fdb_mtx;
		int fdb_expire_time;

		int mtu;		// MTU of ethernet interface
		int tx_size;	// Max size of packet to send
		uint16_t l2mtu; // Internal switch L2 MTU

		// Packet format
		PacketHeaderOptionFormatType packet_format;

		// Source address to bind to
		std::shared_ptr<struct sockaddr_storage> src_addr;

		// TAP interface
		std::unique_ptr<TAPInterface> tap_interface;

		// UDP socket for tunneling
		int udp_socket;

		// Remote nodes
		std::map<std::array<uint8_t, 16>, std::shared_ptr<RemoteNode>> remote_nodes;

		// Port we're using
		int port;

		// RX path
		std::shared_ptr<accl::BufferPool<PacketBuffer>> available_rx_buffer_pool;
		// TX path
		std::shared_ptr<accl::BufferPool<PacketBuffer>> available_tx_buffer_pool;
		std::shared_ptr<accl::BufferPool<PacketBuffer>> tap_write_pool;
		// Threads
		std::unique_ptr<std::thread> tunnel_tap_read_thread;
		std::unique_ptr<std::thread> tunnel_socket_read_thread;
		std::unique_ptr<std::thread> tunnel_tap_write_thread;
		std::unique_ptr<std::thread> fdb_thread;

		// Stop everything running...
		bool stop_flag;

		void tunnel_tap_read_handler();
		void tunnel_socket_read_handler();
		void tunnel_tap_write_handler();
		void fdb_handler();

		void _create_udp_socket();
		void _destroy_udp_socket();
};

uint16_t get_l4mtu(uint16_t max_packet_size, sockaddr_storage *addr);
uint16_t get_l2mtu_from_mtu(uint16_t mtu);
