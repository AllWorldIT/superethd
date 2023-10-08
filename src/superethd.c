

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffers.h"
#include "debug.h"
#include "sockets.h"
#include "tap.h"
#include "threads.h"
#include "tunnel.h"
#include "util.h"

int stop_program = 0;

void handleSIGUSR1(int signum) {
	// Handle SIGUSR1
	FPRINTF("Received SIGUSR1. Exiting...");
	stop_program = 1;
	// exit(EXIT_SUCCESS);
}

int start_set(char *ifname, struct in6_addr *src, struct in6_addr *dst, int port, int mtu, int mss) {
	// Register the signal handler for SIGUSR1
	if (signal(SIGUSR1, handleSIGUSR1) == SIG_ERR) {
		FPRINTF("ERROR: Failed to register signal handler: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Set up thread data
	struct ThreadData tdata;
	// Allocate buffers
	initialize_buffer_list(&tdata.available_buffers, STAP_BUFFER_COUNT, STAP_MAX_PACKET_SIZE);
	// Initialize our queues
	initialize_buffer_list(&tdata.tx_packet_queue, 0, 0);
	initialize_buffer_list(&tdata.rx_packet_queue, 0, 0);

	// Next the other thread information
	tdata.stop_program = &stop_program;

	tdata.local_addr.sin6_addr = *src;
	tdata.local_addr.sin6_family = tdata.remote_addr.sin6_family = AF_INET6;
	tdata.local_addr.sin6_port = tdata.remote_addr.sin6_port = htons(port);

	tdata.local_addr.sin6_addr = *dst;

	// MSS
	tdata.mss = mss;
	if (tdata.mss > STAP_MAX_MSS_SIZE) {
		FPRINTF("ERROR: Maximum MSS is %u", STAP_MAX_MSS_SIZE);
		exit(EXIT_FAILURE);
	}
	if (tdata.mss < STAP_MIN_MSS_SIZE) {
		FPRINTF("ERROR: Minimum MSS is %u", STAP_MIN_MSS_SIZE);
		exit(EXIT_FAILURE);
	}
	// MTU
	tdata.mtu = mtu;
	if (tdata.mtu > STAP_MAX_MTU_SIZE) {
		FPRINTF("ERROR: Maximum MTU is %u!", STAP_MAX_MTU_SIZE);
		exit(EXIT_FAILURE);
	}
	if (tdata.mtu < STAP_MIN_MTU_SIZE) {
		FPRINTF("ERROR: Minimum MTU is %u!", STAP_MIN_MTU_SIZE);
		exit(EXIT_FAILURE);
	}

	// Work out REAL maximum packet payload size
	tdata.max_payload_size = tdata.mss;
	// Ethernet frame size
	tdata.max_payload_size -= 14;
	// Reduce by IP header size
	if (is_ipv4_mapped_ipv6(&tdata.remote_addr.sin6_addr))
		tdata.max_payload_size -= 20;
	else
		tdata.max_payload_size -= 40;
	tdata.max_payload_size -= 8;  // UDP frame

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
	FPRINTF("Enabling ethernet device '%s'...", ifname);
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
