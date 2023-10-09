#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec.h"
#include "debug.h"
#include "threads.h"
#include "util.h"

int main() {
	char *src_value = "fec0::1";

	// Convert local address to a IPv6 sockaddr
	struct in6_addr dst_addr;
	char dst_addr_str[INET6_ADDRSTRLEN];
	if (to_sin6addr(src_value, &dst_addr)) {
		if (inet_ntop(AF_INET6, &dst_addr, dst_addr_str, sizeof(dst_addr_str)) == NULL) {
			FPRINTF("ERROR: Failed to convert address '%s' to IPv6 address: %s", src_value, strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		FPRINTF("ERROR: Failed to convert address '%s' to IPv6 address", src_value);
		exit(EXIT_FAILURE);
	}

	// Maximum transmission packet size
	int max_packet_size = 1500;
	// Maximum SET ethernet device MTU
	int device_mtu = 1500;

	// Work out payload size
	uint16_t max_payload_size = get_max_payload_size(max_packet_size, &dst_addr);
	uint16_t max_ethernet_frame_size = get_max_ethernet_frame_size(device_mtu);
	size_t wbuffer_count = get_codec_wbuffer_count(max_ethernet_frame_size);

	// Set up packet decoder state
	PacketDecoderState state;
	init_packet_decoder(&state, max_packet_size);

	DEBUG_PRINT("Allocating %i wbuffers of %i size each", wbuffer_count, max_ethernet_frame_size);
	BufferList wbuffers;
	initialize_buffer_list(&wbuffers, wbuffer_count, max_ethernet_frame_size);

	return 0;
}
