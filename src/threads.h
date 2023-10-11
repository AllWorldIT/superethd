
#ifndef __THREAD_H__
#define __THREAD_H__

#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>

#include "buffers.h"

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

#endif
