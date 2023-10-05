#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>


#include "common.h"
#include "config.h"
#include "packet.h"
#include "sockets.h"
#include "tap.h"
#include "threads.h"
#include "tunnel.h"
#include "util.h"

#define TAP_INTERFACE_NAME "t0p0"

#include "sys/gmon.h"
#include "buffers.h"


int stop_program = 0;

void handleSIGUSR1(int signum) {
	// Handle SIGUSR1
	printf("Received SIGUSR1. Exiting...\n");
	stop_program = 1;
	//exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[]) {
	if (argc != 5) {
		fprintf(stderr, "Usage: %s <local> <remote> <mss> <mtu>\n", argv[0]);
		exit(1);
	}

	char *local = argv[1];
	char *remote = argv[2];
	char *mss = argv[3];
	char *mtu = argv[4];
	char *port = "1725";

	char ifname[IFNAMSIZ];
	struct ThreadData tdata;


	// Register the signal handler for SIGUSR1
	if (signal(SIGUSR1, handleSIGUSR1) == SIG_ERR) {
		perror("Error registering signal handler");
		exit(EXIT_FAILURE);
	}

	// Set up thread data
	// Allocate buffers
	initialize_buffer_list(&tdata.available_buffers, STAP_BUFFER_COUNT, STAP_MAX_PACKET_SIZE);
	// Initialize our queues
	initialize_buffer_list(&tdata.tx_packet_queue, 0, 0);
	initialize_buffer_list(&tdata.rx_packet_queue, 0, 0);
	// Next the other thread information
	tdata.stop_program = &stop_program;
	tdata.local_addr.sin6_family = tdata.remote_addr.sin6_family = AF_INET6;
	tdata.local_addr.sin6_port = tdata.remote_addr.sin6_port = htons(atoi(port));
	// MSS
	tdata.mss = atoi(mss);
	if (tdata.mss > STAP_MAX_MSS_SIZE) {
		fprintf(stderr, "ERROR: Maximum MSS is %u", STAP_MAX_MSS_SIZE);
		exit(EXIT_FAILURE);
	}
	if (tdata.mss < STAP_MIN_MSS_SIZE) {
		fprintf(stderr, "ERROR: Minimum MSS is %u", STAP_MIN_MSS_SIZE);
		exit(EXIT_FAILURE);
	}
	// MTU
	tdata.mtu = atoi(mtu);
	if (tdata.mtu > STAP_MAX_MTU_SIZE) {
		fprintf(stderr, "ERROR: Maximum MTU is %u!", STAP_MAX_MTU_SIZE);
		exit(EXIT_FAILURE);
	}
	if (tdata.mtu < STAP_MIN_MTU_SIZE) {
		fprintf(stderr, "ERROR: Minimum MTU is %u!", STAP_MIN_MTU_SIZE);
		exit(EXIT_FAILURE);
	}

	// Convert local address to a IPv6 sockaddr
	if (to_sin6addr(local, &(tdata.local_addr.sin6_addr))) {
		char ipv6_str[INET6_ADDRSTRLEN];
		if (inet_ntop(AF_INET6, &tdata.local_addr.sin6_addr, ipv6_str, sizeof(ipv6_str)) == NULL) {
			perror("inet_ntop failed");
			exit(EXIT_FAILURE);
		}
		printf("LOCAL: %s\n", ipv6_str);
	} else {
		printf("ERROR: Failed to convert '%s' to IPv6 address\n", local);
		exit(EXIT_FAILURE);
	}

	// Convert remote address to a IPv6 sockaddr
	if (to_sin6addr(remote, &(tdata.remote_addr.sin6_addr))) {
		char ipv6_str[INET6_ADDRSTRLEN];
		if (inet_ntop(AF_INET6, &tdata.remote_addr.sin6_addr, ipv6_str, sizeof(ipv6_str)) == NULL) {
			perror("inet_ntop failed");
			exit(EXIT_FAILURE);
		}
		printf("REMOTE: %s\n", ipv6_str);
	} else {
		printf("ERROR: Failed to convert '%s' to IPv6 address\n", remote);
		exit(EXIT_FAILURE);
	}

	// Work out REAL maximum packet payload size
	tdata.max_payload_size = tdata.mss;
	// Reduce by IP header size
	if (is_ipv4_mapped_ipv6(&tdata.remote_addr.sin6_addr))
		tdata.max_payload_size -= 20;
	else
		tdata.max_payload_size -= 40;
	tdata.max_payload_size -= 8;  // UDP frame

	// IMPORTANT: MTU of the interface would technically be 14 bytes smaller than the MSS due to the ethernet frame size
	// we need to
	//            account for.

	fprintf(stderr, "MSS SIZE....: %u\n", tdata.mss);
	fprintf(stderr, "MTU SIZE....: %u\n", tdata.mtu);
	fprintf(stderr, "PAYLOAD SIZE: %u\n", tdata.max_payload_size);
	fprintf(stderr, "BUFFER COUNT: %u\n", STAP_BUFFER_COUNT);

	// Setup TAP interface name
	strcpy(ifname, TAP_INTERFACE_NAME);
	// Allocate actual interface
	create_tap_interface(ifname, &tdata);

	fprintf(stderr, "INTERFACE: %s\n", tdata.tap_device.ifname);
	fprintf(stderr, "MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n", tdata.tap_device.hwaddr[0], tdata.tap_device.hwaddr[1],
		   tdata.tap_device.hwaddr[2], tdata.tap_device.hwaddr[3], tdata.tap_device.hwaddr[4],
		   tdata.tap_device.hwaddr[5]);

	// Create UDP socket
	if (create_udp_socket(&tdata) != 0) {
		destroy_tap_interface(&tdata);
		exit(EXIT_FAILURE);
	}

	// Set interface MTU
	fprintf(stderr, "Setting ethernet device MTU...\n");
	set_interface_mtu(&tdata);

	// Create threads for receiving and sending data
	pthread_t tunnel_read_tap_thread, tunnel_write_socket_thread, tunnel_read_socket_thread, tunnel_write_tap_thread;
	if (pthread_create(&tunnel_read_tap_thread, NULL, tunnel_read_tap_handler, &tdata) != 0) {
		perror("Error creating recv thread");
		exit(1);
	}
	if (pthread_create(&tunnel_write_socket_thread, NULL, tunnel_write_socket_handler, &tdata) != 0) {
		perror("Error creating recv thread");
		exit(1);
	}

	if (pthread_create(&tunnel_read_socket_thread, NULL, tunnel_read_socket_handler, &tdata) != 0) {
		perror("Error creating recv thread");
		exit(1);
	}
	if (pthread_create(&tunnel_write_tap_thread, NULL, tunnel_write_tap_handler, &tdata) != 0) {
		perror("Error creating recv thread");
		exit(1);
	}

	// Online interface
	fprintf(stderr, "Enabling ethernet device...\n");
	start_tap_interface(&tdata);

	// Wait for threads to finish (should never happen)
	//pthread_join(send_thread, NULL);
	pthread_join(tunnel_read_tap_thread, NULL);
	pthread_join(tunnel_write_socket_thread, NULL);
	pthread_join(tunnel_read_socket_thread, NULL);
	pthread_join(tunnel_write_tap_thread, NULL);

	fprintf(stderr, "NORMAL EXIT.....");

	// Cleanup
	destroy_udp_socket(&tdata);
	destroy_tap_interface(&tdata);

	return 0;
}
