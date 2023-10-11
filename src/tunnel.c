#include "tunnel.h"

#include <arpa/inet.h>
#include <errno.h>
#include <lz4.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "buffers.h"
#include "common.h"
#include "debug.h"
#include "codec.h"
#include "tap.h"
#include "threads.h"
#include "util.h"

// Reads data from the TAP interface and queues packets for sending over the tunnel.
void *tunnel_read_tap_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	while (1) {
		if (*tdata->stop_program) break;

		// Grab buffer
		BufferNode *buffer_node = get_buffer_node_head(&tdata->available_buffers);
		if (!buffer_node) {
			FPRINTF("Failed to get buffer");
			exit(EXIT_FAILURE);
		}
		// Set the buffer variable so we can access it more easily
		Buffer *buffer = buffer_node->buffer;

		// Read data from TAP interface
		buffer->length = read(tdata->tap_device.fd, buffer->contents, tdata->max_ethernet_frame_size);
		if (buffer->length == -1) {
			FPRINTF("tunnel_read_tap_handler(): Got an error on read(): %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		DEBUG_PRINT("AVAIL Buffer size: %li", get_buffer_list_size(&tdata->available_buffers));
		DEBUG_PRINT("QUEUE Buffer size: %li", get_buffer_list_size(&tdata->tx_packet_queue));

		DEBUG_PRINT("Read %li bytes from TAP", buffer->length);

		// Append buffer node to queue
		append_buffer_node(&tdata->tx_packet_queue, buffer_node);
	}

	return NULL;
}

// Write data from the TX packet queue to the tunnel socket.
void *tunnel_write_socket_handler(void *arg) {
	// Thread data
	struct ThreadData *tdata = (struct ThreadData *)arg;
	// Constant used below
	socklen_t addrlen = sizeof(struct sockaddr_in6);
	// Just a stat
	ssize_t max_queue_size = 0;
	// sendto() flags
	int flags = 0;

	// Packet encoder state
	PacketEncoderState state;
	init_packet_encoder(&state,  tdata->max_ethernet_frame_size);

	// START - packet encoder buffers
	// NK: work out the write node count based on how many packets it takes to split the maximum packet size plus 1
	int wbuffer_count = get_codec_wbuffer_count(tdata->max_ethernet_frame_size);
	DEBUG_PRINT("Allocating %i wbuffers of %i size each", tdata->wbuffer_count, tdata->max_ethernet_frame_size);
	BufferList wbuffers;
	initialize_buffer_list(&wbuffers, wbuffer_count, tdata->max_ethernet_frame_size);

	// Main IO loop
	int buffer_wait_time = 0;
	while (1) {
		if (*tdata->stop_program) break;

		// Grab buffer
		BufferNode *buffer_node;
		int result = get_buffer_node_head_wait(&tdata->tx_packet_queue, &buffer_node, 0, buffer_wait_time);
		// Check for error
		if (result == -1) {
			FPRINTF("pthread_cond_wait() seems to of failed?");
			exit(EXIT_FAILURE);
			// Check for timeout
		} else if (result == 0) {
			DEBUG_PRINT("buffer wait timeout triggered!");
			buffer_wait_time = 0;
		}

		if (!buffer_node) {
			FPRINTF("Failed to get buffer, list probably empty");
			exit(EXIT_FAILURE);
		}

		// Grab buffer list size
		int cur_queue_size = get_buffer_list_size(&tdata->tx_packet_queue);
		if (cur_queue_size > max_queue_size) {
			max_queue_size = cur_queue_size;
			FPRINTF("Max packet queue size %li", max_queue_size);
		}

		// TODO: loop with queue size to allow chunking of packets, we probably want to start with a clean state, with a flush state

		// Encode packet and get number of resulting packets generated, these are stored in our wbuffers
		int pktcount = packet_encoder(&state, &wbuffers, buffer_node->buffer, tdata->max_payload_size);
		DEBUG_PRINT("Got %i packets back from packet encoder", pktcount);
		// Add buffer back to available list
		append_buffer_node(&tdata->available_buffers, buffer_node);

		// Loop with our buffers and write them out
		BufferNode *wbuffer_node = wbuffers.head;
		for (int i = 0; i < pktcount; ++i) {
			// Write data out
			ssize_t bytes_written = sendto(tdata->udp_socket, wbuffer_node->buffer->contents, wbuffer_node->buffer->length, flags,
										   (struct sockaddr *)&tdata->remote_addr, addrlen);
			if (bytes_written == -1) {
				FPRINTF("Got an error on sendto(): %s", strerror(errno));
				exit(EXIT_FAILURE);
			}
			DEBUG_PRINT("Wrote %li bytes to tunnel [%i/%i]", bytes_written, i + 1, pktcount);
			// Go to next buffer
			wbuffer_node = wbuffer_node->next;
		}

		// Reset packet encoder after writing out wbuffers
		reset_packet_encoder(&state);
	}

	return NULL;
}

/**
 * @brief Tunnel read thread.
 *
 * Reads data from the tunnel socket and queues them to be delivered over the TAP interface.
 *
 * @param arg Thread data.
 * @return void*
 */
void *tunnel_read_socket_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	struct sockaddr_in6 *controls;
	ssize_t sockaddr_len = sizeof(struct sockaddr_in6);
	BufferNode **buffer_nodes;
	struct mmsghdr *msgs;
	struct iovec *iovecs;
	PacketHeader *pkthdr;

	// START - IO vector buffers

	// Allocate memory for our buffers and IO vectors
	buffer_nodes = (BufferNode **)malloc(SETH_MAX_RECVMM_MESSAGES * sizeof(BufferNode));
	msgs = (struct mmsghdr *)malloc(SETH_MAX_RECVMM_MESSAGES * sizeof(struct mmsghdr));
	iovecs = (struct iovec *)malloc(SETH_MAX_RECVMM_MESSAGES * sizeof(struct iovec));
	controls = (struct sockaddr_in6 *)calloc(1, SETH_MAX_RECVMM_MESSAGES * sizeof(struct sockaddr_in6));
	// Make sure memory allocation succeeded
	if (msgs == NULL || iovecs == NULL || controls == NULL || buffer_nodes == NULL) {
		FPRINTF("Memory allocation failed: %s", strerror(errno));
		// Clean up previously allocated memory
		free(buffer_nodes);
		free(msgs);
		free(iovecs);
		free(controls);
		exit(EXIT_FAILURE);
	}
	// Set up iovecs and mmsghdrs
	for (int i = 0; i < SETH_MAX_RECVMM_MESSAGES; ++i) {
		buffer_nodes[i] = get_buffer_node_head(&tdata->available_buffers);
		if (buffer_nodes[i] == NULL) {
			perror("%s(): Could not get available buffer node");
			// Clean up allocated memory
			for (int j = 0; j < i; ++j) {
				append_buffer_node(&tdata->available_buffers, buffer_nodes[i]);
			}
			free(buffer_nodes);
			free(msgs);
			free(iovecs);
			free(controls);
			return NULL;
		}
		// Setup IO vectors
		iovecs[i].iov_base = buffer_nodes[i]->buffer->contents;
		iovecs[i].iov_len = tdata->max_ethernet_frame_size;
		// And the associated msgs
		msgs[i].msg_hdr.msg_iov = &iovecs[i];
		msgs[i].msg_hdr.msg_iovlen = 1;
		msgs[i].msg_hdr.msg_name = (void *)&controls[i];
		msgs[i].msg_hdr.msg_namelen = sockaddr_len;
	}
	// END - IO vector buffers

	while (1) {
		if (*tdata->stop_program) break;

		int num_received = recvmmsg(tdata->udp_socket, msgs, SETH_MAX_RECVMM_MESSAGES, MSG_WAITFORONE, NULL);
		if (num_received == -1) {
			FPRINTF("recvmmsg failed: %s", strerror(errno));
			break;
		}

		for (int i = 0; i < num_received; ++i) {
			char ipv6_str[INET6_ADDRSTRLEN];
			if (inet_ntop(AF_INET6, &controls[i].sin6_addr, ipv6_str, sizeof(ipv6_str)) == NULL) {
				FPRINTF("inet_ntop failed: %s", strerror(errno));
				exit(EXIT_FAILURE);
			}
			DEBUG_PRINT("Received %u bytes from %s:%d with flags %i", msgs[i].msg_len, ipv6_str, ntohs(controls[i].sin6_port),
						msgs[i].msg_hdr.msg_flags);
			// Compare to see if this is the remote system IP
			if (memcmp(&tdata->remote_addr.sin6_addr, &controls[i].sin6_addr, sizeof(struct in6_addr))) {
				FPRINTF("Source address is incorrect %s!", ipv6_str);
				continue;
			}

			// Grab this IOV's buffer node and update the actual buffer length
			BufferNode *buffer_node = buffer_nodes[i];
			buffer_node->buffer->length = msgs[i].msg_len;

			// Make sure the buffer is long enough for us to overlay the header
			if (buffer_node->buffer->length < SETH_PACKET_HEADER_SIZE) {
				FPRINTF("Packet too small %lu < 12, DROPPING!!!", buffer_node->buffer->length);
				continue;
			}

			// Overlay packet header and do basic header checks
			pkthdr = (PacketHeader *)buffer_node->buffer->contents;
			// Check version is supported
			if (pkthdr->ver > SETH_PACKET_VERSION_V1) {
				FPRINTF("Packet not supported, version %i vs. our version %i, DROPPING!", pkthdr->ver, SETH_PACKET_VERSION_V1);
				continue;
			}
			if (pkthdr->reserved != 0) {
				FPRINTF("Packet header should not have any reserved its set, it is %u, DROPPING!", pkthdr->reserved);
				continue;
			}
			// First thing we do is validate the header
			int valid_format_mask = SETH_PACKET_FORMAT_ENCAP | SETH_PACKET_FORMAT_COMPRESSED;
			if (pkthdr->format & ~valid_format_mask) {
				FPRINTF("Packet in invalid format %x, DROPPING!", pkthdr->format);
				continue;
			}
			// Next check the channel is set to 1
			if (pkthdr->channel != 1) {
				FPRINTF("Packet specifies invalid channel %d, DROPPING!", pkthdr->channel);
				continue;
			}

			// Add buffer node to RX queue
			append_buffer_node(&tdata->rx_packet_queue, buffer_node);

			// Allocate new buffer node to replace this one
			buffer_node = get_buffer_node_head(&tdata->available_buffers);
			buffer_nodes[i] = buffer_node;
			msgs[i].msg_hdr.msg_iov->iov_base = buffer_node->buffer->contents;
		}
	}

	// Add buffers back to available list for de-allocation
	for (int i = 0; i < SETH_MAX_RECVMM_MESSAGES; ++i) {
		append_buffer_node(&tdata->available_buffers, buffer_nodes[i]);
	}
	// Free the rest of the IOV stuff
	free(buffer_nodes);
	free(msgs);
	free(iovecs);

	return NULL;
}

/**
 * @brief Ethernet device write thread.
 *
 * Process packets in the RX queue, decode and send them to the TAP device.
 *
 * @param arg Thread data.
 * @return void*
 */
void *tunnel_write_tap_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;
	ssize_t max_queue_size = 0;

	// Set up packet decoder state
	PacketDecoderState state;
	init_packet_decoder(&state, tdata->max_ethernet_frame_size);

	// START - packet decoder buffers
	// NK: work out the write node count based on how many packets it takes to split the maximum packet size plus 1
	int wbuffer_count = get_codec_wbuffer_count(tdata->max_ethernet_frame_size);
	DEBUG_PRINT("Allocating %i wbuffers of %i size each", tdata->wbuffer_count, tdata->max_ethernet_frame_size);
	BufferList wbuffers;
	initialize_buffer_list(&wbuffers, wbuffer_count, tdata->max_ethernet_frame_size);
	// END - packet decoder buffers

	while (1) {
		if (*tdata->stop_program) break;

		// Grab buffer
		BufferNode *buffer_node;
		int result = get_buffer_node_head_wait(&tdata->rx_packet_queue, &buffer_node, 0, 0);
		// Check we actually got something
		if (result != 0) {
			FPRINTF("pthread_cond_wait() seems to of failed?");
			exit(EXIT_FAILURE);
		}
		if (!buffer_node) {
			FPRINTF("Failed to get buffer, list probably empty");
			exit(EXIT_FAILURE);
		}

		// Grab buffer list size
		int cur_queue_size = get_buffer_list_size(&tdata->rx_packet_queue);
		if (cur_queue_size > max_queue_size) {
			max_queue_size = cur_queue_size;
			FPRINTF("Max RX packet queue size %li", max_queue_size);
		}

		// Encode packet and get number of resulting packets generated, these are stored in our wbuffers
		int pktcount = packet_decoder(&state, &wbuffers, buffer_node->buffer, tdata->mtu);
		DEBUG_PRINT("Got %i packets back from packet decoder", pktcount);
		// Add buffer back to available list
		append_buffer_node(&tdata->available_buffers, buffer_node);

		// Loop with our buffers and write them out
		BufferNode *wbuffer_node = wbuffers.head;
		for (int i = 0; i < pktcount; ++i) {
			// Write data to TAP interface
			ssize_t bytes_written = write(tdata->tap_device.fd, wbuffer_node->buffer->contents, wbuffer_node->buffer->length);
			if (bytes_written == -1) {
				FPRINTF("Error writing TAP device: %s", strerror(errno));
				exit(EXIT_FAILURE);
			}
			DEBUG_PRINT("Wrote %li bytes to TAP [%i/%i]", bytes_written, i + 1, pktcount);
			// Go to next buffer
			wbuffer_node = wbuffer_node->next;
		}
	}

	return NULL;
}
