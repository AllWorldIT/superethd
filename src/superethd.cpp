/*
 * Super Ethernet Tunnel daemon handling.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string>
extern "C" {
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include "buffers.hpp"
#include "debug.hpp"
#include "sockets.hpp"
#include "tap.hpp"
#include "threads.hpp"
#include "tunnel.hpp"
#include "util.hpp"

int stop_program = 0;

void handleSIGUSR1(int signum) {
	// Handle SIGUSR1
	FPRINTF("Received SIGUSR1. Exiting...");
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
		FPRINTF("ERROR: Failed to register signal handler: %s", strerror(errno));
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

	// MSS
	tdata.tx_size = tx_size;
	if (tdata.tx_size > SETH_MAX_TXSIZE) {
		FPRINTF("ERROR: Maximum TX_SIZE is %i", SETH_MAX_TXSIZE);
		exit(EXIT_FAILURE);
	}
	if (tdata.tx_size < SETH_MIN_TXSIZE) {
		FPRINTF("ERROR: Minimum MSS is %i", SETH_MIN_TXSIZE);
		exit(EXIT_FAILURE);
	}
	// MTU
	tdata.mtu = mtu;
	if (tdata.mtu > SETH_MAX_MTU_SIZE) {
		FPRINTF("ERROR: Maximum MTU is %i!", SETH_MAX_MTU_SIZE);
		exit(EXIT_FAILURE);
	}
	if (tdata.mtu < SETH_MIN_MTU_SIZE) {
		FPRINTF("ERROR: Minimum MTU is %i!", SETH_MIN_MTU_SIZE);
		exit(EXIT_FAILURE);
	}

	// Get maximum ethernet frame size we can get
	tdata.max_ethernet_frame_size = get_max_ethernet_frame_size(tdata.mtu);

	// Get REAL maximum packet payload size
	tdata.max_payload_size = get_max_payload_size(tdata.tx_size, &tdata.remote_addr.sin6_addr);
	FPRINTF("Setting maximum UDP payload size to %i...", tdata.max_payload_size);

	// Allocate buffers
	initialize_buffer_list(&tdata.available_buffers, SETH_BUFFER_COUNT, tdata.max_ethernet_frame_size);
	// Initialize our queues
	initialize_buffer_list(&tdata.tx_packet_queue, 0, 0);
	initialize_buffer_list(&tdata.rx_packet_queue, 0, 0);

	/*
	 * End thread data setup
	 */

	// Allocate actual interface
	create_tap_interface(ifname, &tdata);

	FPRINTF("Assigned MAC address: %02x:%02x:%02x:%02x:%02x:%02x", tdata.tap_device.hwaddr[0], tdata.tap_device.hwaddr[1],
			tdata.tap_device.hwaddr[2], tdata.tap_device.hwaddr[3], tdata.tap_device.hwaddr[4], tdata.tap_device.hwaddr[5]);

	// Create UDP socket
	if (create_udp_socket(&tdata) != 0) {
		destroy_tap_interface(&tdata);
		exit(EXIT_FAILURE);
	}

	// Set interface MTU
	FPRINTF("Setting ethernet device MTU...");
	set_interface_mtu(&tdata);

	// Create threads for receiving and sending data
	pthread_t tunnel_read_tap_thread, tunnel_write_socket_thread, tunnel_read_socket_thread, tunnel_write_tap_thread;
	if (pthread_create(&tunnel_read_tap_thread, NULL, tunnel_read_tap_handler, &tdata) != 0) {
		FPRINTF("ERROR: Failed to create tunnel_read_tap_thread thread: %s", strerror(errno));
		exit(1);
	}
	if (pthread_create(&tunnel_write_socket_thread, NULL, tunnel_write_socket_handler, &tdata) != 0) {
		FPRINTF("ERROR: Failed to create tunnel_write_socket_thread thread: %s", strerror(errno));
		exit(1);
	}

	if (pthread_create(&tunnel_read_socket_thread, NULL, tunnel_read_socket_handler, &tdata) != 0) {
		FPRINTF("ERROR: Failed to create tunnel_read_socket_thread thread: %s", strerror(errno));
		exit(1);
	}
	if (pthread_create(&tunnel_write_tap_thread, NULL, tunnel_write_tap_handler, &tdata) != 0) {
		FPRINTF("ERROR: Failed to create tunnel_write_tap_thread thread: %s", strerror(errno));
		exit(1);
	}

	// Online interface
	FPRINTF("Enabling ethernet device '%s'...", ifname.c_str());
	start_tap_interface(&tdata);

	// Wait for threads to finish (should never happen)
	// pthread_join(send_thread, NULL);
	pthread_join(tunnel_read_tap_thread, NULL);
	pthread_join(tunnel_write_socket_thread, NULL);
	pthread_join(tunnel_read_socket_thread, NULL);
	pthread_join(tunnel_write_tap_thread, NULL);

	FPRINTF("NORMAL EXIT.....");

	// Cleanup
	destroy_udp_socket(&tdata);
	destroy_tap_interface(&tdata);

	return 0;
}
