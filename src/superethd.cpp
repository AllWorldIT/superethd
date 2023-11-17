/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "common.hpp"
#include "libaccl/Buffer.hpp"
#include "libaccl/BufferPool.hpp"
#include "libaccl/Logger.hpp"

#include <errno.h>
#include <signal.h>
#include <string.h>

#include "debug.hpp"
#include "sockets.hpp"
#include "tap.hpp"
#include "threads.hpp"
#include "tunnel.hpp"
#include "util.hpp"

int stop_program = 0;

void handleSIGUSR1(int signum) {
	// Handle SIGUSR1
	CERR("Received SIGUSR1. Exiting...");
	stop_program = 1;
	// exit(EXIT_SUCCESS);
}

/**
 * @brief Start Super Ethernet Tunnel.
 *
 * @param ifname Interface name.
 * @param src Source IP address for packets.
 * @param dst Destination IP address for packets.
 * @param port Port to use for communication.
 * @param mtu SET ethernet device MTU.
 * @param tx_size Maximum transmission packet size.
 * @return int
 */
int start_set(const std::string ifname, struct in6_addr *src, struct in6_addr *dst, int port, int mtu, int tx_size) {
	// Register the signal handler for SIGUSR1
	if (signal(SIGUSR1, handleSIGUSR1) == SIG_ERR) {
		CERR("ERROR: Failed to register signal handler: {}", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/*
	 * Start thread data setup
	 */

	struct ThreadData tdata;

	// Next the other thread information
	tdata.stop_program = &stop_program;

	// Tunnel info
	tdata.local_addr.sin6_addr = *src;
	tdata.local_addr.sin6_family = tdata.remote_addr.sin6_family = AF_INET6;
	tdata.local_addr.sin6_port = tdata.remote_addr.sin6_port = htons(port);
	tdata.remote_addr.sin6_addr = *dst;

	// MTU
	tdata.mtu = mtu;
	if (tdata.mtu > SETH_MAX_MTU_SIZE) {
		CERR("ERROR: Maximum MTU is {}!", SETH_MAX_MTU_SIZE);
		exit(EXIT_FAILURE);
	}
	if (tdata.mtu < SETH_MIN_MTU_SIZE) {
		CERR("ERROR: Minimum MTU is {}!", SETH_MIN_MTU_SIZE);
		exit(EXIT_FAILURE);
	}

	// Maximum TX size
	tdata.tx_size = tx_size;
	if (tdata.tx_size > SETH_MAX_TXSIZE) {
		CERR("ERROR: Maximum TX_SIZE is {}", SETH_MAX_TXSIZE);
		exit(EXIT_FAILURE);
	}
	if (tdata.tx_size < SETH_MIN_TXSIZE) {
		CERR("ERROR: Minimum TX_SIZE is {}", SETH_MIN_TXSIZE);
		exit(EXIT_FAILURE);
	}
	if (tdata.tx_size > tdata.mtu) {
		CERR("ERROR: TX_SIZE {} cannot be greater than MTU is {}", tdata.tx_size, tdata.mtu);
		exit(EXIT_FAILURE);
	}

	// Get maximum ethernet frame size we can get
	tdata.l2mtu = get_l2mtu_from_mtu(tdata.mtu);

	// Get REAL maximum packet payload size
	tdata.l4mtu = get_l4mtu(tdata.tx_size, &tdata.remote_addr.sin6_addr);
	CERR("Setting maximum payload size to {}...", tdata.l4mtu);

	/*
	 * End thread data setup
	 */

	// Allocate actual interface
	create_tap_interface(ifname, &tdata);

	CERR("Assigned MAC address: {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", tdata.tap_device.hwaddr[0], tdata.tap_device.hwaddr[1],
		 tdata.tap_device.hwaddr[2], tdata.tap_device.hwaddr[3], tdata.tap_device.hwaddr[4], tdata.tap_device.hwaddr[5]);

	// Create UDP socket
	if (create_udp_socket(&tdata) != 0) {
		destroy_tap_interface(&tdata);
		exit(EXIT_FAILURE);
	}

	// Set interface MTU
	CERR("Setting ethernet device MTU...");
	set_interface_mtu(&tdata);

	// Initialize threads
	std::thread tunnel_read_tap_thread(tunnel_read_tap_handler, &tdata);
	std::thread tunnel_read_socket_thread(tunnel_read_socket_handler, &tdata);

	// Online interface
	CERR("Enabling ethernet device '{}'...", ifname.c_str());
	start_tap_interface(&tdata);

	// Join all threads after the interface comes online
	tunnel_read_tap_thread.join();
	tunnel_read_socket_thread.join();

	CERR("NORMAL EXIT.....");

	// Cleanup
	destroy_udp_socket(&tdata);
	destroy_tap_interface(&tdata);

	return 0;
}
