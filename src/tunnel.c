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
#include "packet.h"
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
			fprintf(stderr, "%s(): Failed to get buffer\n", __func__);
			exit(EXIT_FAILURE);
		}
		// Set the buffer variable so we can access it more easily
		Buffer *buffer = buffer_node->buffer;

		// Read data from TAP interface
		buffer->length = read(tdata->tap_device.fd, buffer->contents, STAP_MAX_PACKET_SIZE);
		if (buffer->length == -1) {
			fprintf(stderr, "%s(): tunnel_read_tap_handler(): Got an error on read(): %s\n", __func__, strerror(errno));
			exit(EXIT_FAILURE);
		}

		DEBUG_PRINT("AVAIL Buffer size: %li\n", get_buffer_list_size(&tdata->available_buffers));
		DEBUG_PRINT("QUEUE Buffer size: %li\n", get_buffer_list_size(&tdata->tx_packet_queue));

		DEBUG_PRINT("Read %li bytes from TAP\n", buffer->length);

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
	// Packet sequence number
	uint32_t seq = 0;
	// sendto() flags
	int flags = 0;

	// START - packet encoder buffers
	// NK: work out the write node count based on how many packets it takes to split the maximum packet size plus 1
	int wbuffer_count = ((STAP_MAX_PACKET_SIZE + tdata->mss - 1) / tdata->mss) + 1;
	BufferList wbuffers;
	initialize_buffer_list(&wbuffers, wbuffer_count, STAP_BUFFER_SIZE);
	// Allocate buffer used for compression
	Buffer cbuffer;
	cbuffer.contents = (char *)malloc(STAP_BUFFER_SIZE_COMPRESSION);
	// END - packet encoder buffers

	// Main IO loop
	while (1) {
		if (*tdata->stop_program) break;

		// Grab buffer
		BufferNode *buffer_node;
		int result = get_buffer_node_head_wait(&tdata->tx_packet_queue, &buffer_node, 0, 0);
		// Check we actually got something
		if (result != 0) {
			fprintf(stderr, "%s(): pthread_cond_wait() seems to of failed?\n", __func__);
			exit(EXIT_FAILURE);
		}
		if (!buffer_node) {
			fprintf(stderr, "%s(): Failed to get buffer, list probably empty\n", __func__);
			exit(EXIT_FAILURE);
		}

		// Grab buffer list size
		int cur_queue_size = get_buffer_list_size(&tdata->tx_packet_queue);
		if (cur_queue_size > max_queue_size) {
			max_queue_size = cur_queue_size;
			fprintf(stderr, "%s(): Max packet queue size %li\n", __func__, max_queue_size);
		}

		// TODO: loop with queue size to allow chunking of packets, we probably want to start with a clean state, with a flush state

		// Encode packet and get number of resulting packets generated, these are stored in our wbuffers
		int pktcount = packet_encoder(&wbuffers, &cbuffer, buffer_node->buffer, tdata->mss, seq);
		DEBUG_PRINT("Got %i packets back from packet encoder\n", pktcount);
		// Add buffer back to available list
		append_buffer_node(&tdata->available_buffers, buffer_node);

		// Loop with our buffers and write them out
		BufferNode *wbuffer_node = wbuffers.head;
		for (int i = 0; i < pktcount; ++i) {
			// Write data out
			ssize_t bytes_written = sendto(tdata->udp_socket, wbuffer_node->buffer->contents, wbuffer_node->buffer->length, flags,
										   (struct sockaddr *)&tdata->remote_addr, addrlen);
			if (bytes_written == -1) {
				fprintf(stderr, "%s(): Got an error on sendto(): %s", __func__, strerror(errno));
				exit(EXIT_FAILURE);
			}
			DEBUG_PRINT("Wrote %li bytes to tunnel [%i/%i]\n", bytes_written, i + 1, pktcount);
			// Go to next buffer
			wbuffer_node = wbuffer_node->next;
		}

		// Check if the counter is wrapping
		if (seq == UINT32_MAX)
			seq = 0;
		else
			seq += pktcount;
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
	buffer_nodes = (BufferNode **)malloc(STAP_MAX_RECVMM_MESSAGES * sizeof(BufferNode));
	msgs = (struct mmsghdr *)malloc(STAP_MAX_RECVMM_MESSAGES * sizeof(struct mmsghdr));
	iovecs = (struct iovec *)malloc(STAP_MAX_RECVMM_MESSAGES * sizeof(struct iovec));
	controls = (struct sockaddr_in6 *)calloc(1, STAP_MAX_RECVMM_MESSAGES * sizeof(struct sockaddr_in6));
	// Make sure memory allocation succeeded
	if (msgs == NULL || iovecs == NULL || controls == NULL || buffer_nodes == NULL) {
		fprintf(stderr, "%s(): Memory allocation failed: %s", __func__, strerror(errno));
		// Clean up previously allocated memory
		free(buffer_nodes);
		free(msgs);
		free(iovecs);
		free(controls);
		exit(EXIT_FAILURE);
	}
	// Set up iovecs and mmsghdrs
	for (int i = 0; i < STAP_MAX_RECVMM_MESSAGES; ++i) {
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
		iovecs[i].iov_len = STAP_MAX_PACKET_SIZE;
		// And the associated msgs
		msgs[i].msg_hdr.msg_iov = &iovecs[i];
		msgs[i].msg_hdr.msg_iovlen = 1;
		msgs[i].msg_hdr.msg_name = (void *)&controls[i];
		msgs[i].msg_hdr.msg_namelen = sockaddr_len;
	}
	// END - IO vector buffers

	while (1) {
		if (*tdata->stop_program) break;

		int num_received = recvmmsg(tdata->udp_socket, msgs, STAP_MAX_RECVMM_MESSAGES, MSG_WAITFORONE, NULL);
		// fprintf(stderr, "Read something\n");
		if (num_received == -1) {
			fprintf(stderr, "%s(): recvmmsg failed: %s", __func__, strerror(errno));
			break;
		}

		for (int i = 0; i < num_received; ++i) {
			char ipv6_str[INET6_ADDRSTRLEN];
			if (inet_ntop(AF_INET6, &controls[i].sin6_addr, ipv6_str, sizeof(ipv6_str)) == NULL) {
				fprintf(stderr, "%s(): inet_ntop failed: %s", __func__, strerror(errno));
				exit(EXIT_FAILURE);
			}
			DEBUG_PRINT("Received %u bytes from %s:%d with flags %i\n", msgs[i].msg_len, ipv6_str, ntohs(controls[i].sin6_port),
						msgs[i].msg_hdr.msg_flags);
			// Compare to see if this is the remote system IP
			if (memcmp(&tdata->remote_addr.sin6_addr, &controls[i].sin6_addr, sizeof(struct in6_addr))) {
				fprintf(stderr, "%s(): Source address is incorrect %s!", __func__, ipv6_str);
				continue;
			}

			// Grab this IOV's buffer node and update the actual buffer length
			BufferNode *buffer_node = buffer_nodes[i];
			buffer_node->buffer->length = msgs[i].msg_len;

			// Overlay packet header and do basic header checks
			pkthdr = (PacketHeader *)buffer_node->buffer->contents;
			if (pkthdr->reserved != 0) {
				fprintf(stderr, "%s(): Packet should not have any reserved its set, it is %u, DROPPING!", __func__,
						pkthdr->opt_len);
				continue;
			}
			// First thing we do is validate the header
			int valid_format_mask = STAP_PACKET_FORMAT_ENCAP | STAP_PACKET_FORMAT_COMPRESSED;
			if (pkthdr->format & ~valid_format_mask) {
				fprintf(stderr, "%s(): Packet in invalid format %x, DROPPING!", __func__, pkthdr->format);
				continue;
			}
			// Next check the channel is set to 1
			if (pkthdr->channel != 1) {
				fprintf(stderr, "%s(): Packet specifies invalid channel %d, DROPPING!", __func__, pkthdr->channel);
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
	for (int i = 0; i < STAP_MAX_RECVMM_MESSAGES; ++i) {
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
	PacketDecoderState state = {.first_packet = 1, .last_seq = 0};

	// START - packet decoder buffers
	// NK: work out the write node count based on how many packets it takes to split the maximum packet size plus 1
	int wbuffer_count = ((STAP_MAX_PACKET_SIZE + tdata->mss - 1) / tdata->mss) + 1;
	BufferList wbuffers;
	initialize_buffer_list(&wbuffers, wbuffer_count, STAP_BUFFER_SIZE);
	// Allocate buffer used for compression
	Buffer cbuffer;
	cbuffer.contents = (char *)malloc(STAP_BUFFER_SIZE_COMPRESSION);
	// END - packet decoder buffers

	// TODO: init decoder state   packet_decoder_init(&state)

	while (1) {
		if (*tdata->stop_program) break;

		// Grab buffer
		BufferNode *buffer_node;
		int result = get_buffer_node_head_wait(&tdata->rx_packet_queue, &buffer_node, 0, 0);
		// Check we actually got something
		if (result != 0) {
			fprintf(stderr, "%s(): pthread_cond_wait() seems to of failed?", __func__);
			exit(EXIT_FAILURE);
		}
		if (!buffer_node) {
			fprintf(stderr, "%s(): Failed to get buffer, list probably empty", __func__);
			exit(EXIT_FAILURE);
		}

		// Grab buffer list size
		int cur_queue_size = get_buffer_list_size(&tdata->rx_packet_queue);
		if (cur_queue_size > max_queue_size) {
			max_queue_size = cur_queue_size;
			fprintf(stderr, "%s(): Max RX packet queue size %li\n", __func__, max_queue_size);
		}

		// Encode packet and get number of resulting packets generated, these are stored in our wbuffers
		int pktcount = packet_decoder(&state, &wbuffers, &cbuffer, buffer_node->buffer, tdata->mtu);
		// Add buffer back to available list
		append_buffer_node(&tdata->available_buffers, buffer_node);

		// Loop with our buffers and write them out
		BufferNode *wbuffer_node = wbuffers.head;
		for (int i = 0; i < pktcount; ++i) {
			// Write data to TAP interface
			ssize_t bytes_written = write(tdata->tap_device.fd, wbuffer_node->buffer->contents, wbuffer_node->buffer->length);
			if (bytes_written == -1) {
				fprintf(stderr, "%s(): Error writing TAP device: %s", __func__, strerror(errno));
				exit(EXIT_FAILURE);
			}
			DEBUG_PRINT("Wrote %li bytes to TAP [%i/%i]\n", bytes_written, i + 1, pktcount);
			// Go to next buffer
			wbuffer_node = wbuffer_node->next;
		}
	}

	return NULL;
}
