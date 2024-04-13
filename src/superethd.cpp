/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "codec.hpp"
#include "exceptions.hpp"
#include "libaccl/logger.hpp"
#include "packet_switch.hpp"
#include "threads.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <format>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <signal.h>
#include <sys/resource.h>

// Global packet switch, so we can interface with it from the signal handler
std::unique_ptr<PacketSwitch> global_packet_switch;

/**
 * @brief Handler for SIGUSR1.
 *
 * @param signum Signal number.
 */
void handleSIGUSR1(int signum) {
	// Handle SIGUSR1
	std::cerr << "Received SIGUSR1. Exiting..." << std::endl;
	global_packet_switch->stop();
}

/**
 * @brief Start Super Ethernet Tunnel.
 *
 * @param ifname Interface name.
 * @param src_addr Source IP address for packets.
 * @param dst_addrs Destination IP addresses for nodes we're communicating with.
 * @param port Port to use for communication.
 * @param mtu SET ethernet device MTU.
 * @param tx_size Maximum transmission packet size.
 * @param packet_format Packet format.
 * @return int
 */
int start_seth(const std::string ifname, int mtu, int tx_size, PacketHeaderOptionFormatType packet_format,
			   std::shared_ptr<struct sockaddr_storage> src_addr, std::vector<std::shared_ptr<struct sockaddr_storage>> dst_addrs,
			   int port) {

	// Register the signal handler for SIGUSR1
	if (signal(SIGUSR1, handleSIGUSR1) == SIG_ERR) {
		std::cerr << std::format("ERROR: Failed to register signal handler: {}", strerror(errno)) << std::endl;
		exit(EXIT_FAILURE);
	}

	// Initialize packet switch
	try {
		global_packet_switch = std::make_unique<PacketSwitch>(ifname, mtu, tx_size, packet_format, src_addr, dst_addrs, port);
	} catch (SuperEthernetTunnelException &e) {
		std::cerr << std::format("Failed to initialize packet switch: {}", e.what()) << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cerr << std::format("MTU Size (interface)     : {}", global_packet_switch->getMTUSize()) << std::endl;
	std::cerr << std::format("L2MTU Size (switch)      : {}", global_packet_switch->getL2MTUSize()) << std::endl;

	/*
	 * End thread data setup
	 */
	std::cerr << std::format("Packet format            : {}",
							 PacketHeaderOptionFormatTypeToString(global_packet_switch->getPacketFormat()))
			  << std::endl;

	// Start the packet switch
	global_packet_switch->start();

	// Wait for packet switch to exit, if it ever does
	global_packet_switch->wait();

	LOG_NOTICE("NORMAL EXIT");

	return 0;
}
