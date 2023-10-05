#include "tunnel.h"

#include <arpa/inet.h>
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
#include "packet.h"
#include "tap.h"
#include "threads.h"
#include "util.h"

void *tunnel_read_tap_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	while (1) {
		if (*tdata->stop_program) break;

		// Grab buffer
		BufferNode *buffer_node = get_buffer_node_head(&tdata->available_buffers);
		if (!buffer_node) {
			fprintf(stderr, "tunnel_read_tap_handler(): Failed to get buffer");
			exit(EXIT_FAILURE);
		}
		// Set the buffer variable so we can access it more easily
		Buffer *buffer = buffer_node->buffer;

		// Clear packet header
		memset((void *) buffer->contents, 0, STAP_PACKET_HEADER_SIZE);

		// Read data from TAP interface
		DEBUG_PRINT("%s: before read\n", __func__);
		buffer->length =
			read(tdata->tap_device.fd, buffer->contents + STAP_PACKET_HEADER_SIZE, STAP_MAX_PACKET_SIZE - STAP_PACKET_HEADER_SIZE);
		if (buffer->length == -1) {
			perror("tunnel_read_tap_handler(): Got an error on read()");
			exit(EXIT_FAILURE);
		}

		DEBUG_PRINT("%s: AVAIL Buffer size: %li\n", __func__, get_buffer_list_size(&tdata->available_buffers));
		DEBUG_PRINT("%s: QUEUE Buffer size: %li\n", __func__, get_buffer_list_size(&tdata->tx_packet_queue));

		DEBUG_PRINT("tunnel_read_tap_handler(): Read %li bytes from TAP\n", buffer->length);

		if (buffer->length > tdata->mss) {
			fprintf(stderr, "tunnel_read_tap_handler(): Ethernet frame %li > %u MSS, DROPPING!!!!", buffer->length, tdata->mss);
			append_buffer_node(&tdata->available_buffers, buffer_node);
			continue;
		}

		// Bump the buffer length with the header size
		buffer->length += STAP_PACKET_HEADER_SIZE;

		// Append buffer node to queue
		append_buffer_node(&tdata->tx_packet_queue, buffer_node);
	}

	return NULL;
}

void *tunnel_write_socket_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;
	socklen_t addrlen = sizeof(struct sockaddr_in6);
	PacketHeader *pkthdr;

	ssize_t max_queue_size = 0;
	int flags = 0;
	uint32_t seq = 0;

	while (1) {
		if (*tdata->stop_program) break;

		// Grab buffer
		BufferNode *buffer_node;
		int result = get_buffer_node_head_wait(&tdata->tx_packet_queue, &buffer_node, 0, 0);
		// Grab buffer list size
		int cur_queue_size = get_buffer_list_size(&tdata->tx_packet_queue);
		if (cur_queue_size > max_queue_size) {
			max_queue_size = cur_queue_size;
			fprintf(stderr, "tunnel_write_socket_handler(): Max packet queue size %li\n", max_queue_size);
		}
		// Check we actually got something
		if (result != 0) {
			fprintf(stderr, "tunnel_write_socket_handler(): pthread_cond_wait() seems to of failed?");
			exit(EXIT_FAILURE);
		}
		if (!buffer_node) {
			fprintf(stderr, "tunnel_write_socket_handler(): Failed to get buffer, list probably empty");
			exit(EXIT_FAILURE);
		}
		// Set the buffer variable so we can access it more easily
		Buffer *buffer = buffer_node->buffer;

		// Set up packet header, we point it to the buffer
		pkthdr = (PacketHeader *)buffer->contents;
		// Build packet header, all fields were blanked in the reader
		pkthdr->ver = STAP_PACKET_VERSION_V1;
		pkthdr->format = STAP_PACKET_FORMAT_ENCAP;
		pkthdr->channel = 1;
		pkthdr->sequence = stap_cpu_to_be_32(seq);

		// We keep the data locked while we do our write though
		ssize_t bytes_written =
			sendto(tdata->udp_socket, buffer->contents, buffer->length, flags, (struct sockaddr *)&tdata->remote_addr, addrlen);
		if (bytes_written == -1) {
			perror("tunnel_write_socket_handler(): Got an error on sendto()");
			exit(EXIT_FAILURE);
		}
		DEBUG_PRINT("tunnel_write_socket_handler(): Wrote %li bytes to tunnel\n", bytes_written);

		// Add buffer back to available list
		append_buffer_node(&tdata->available_buffers, buffer_node);

		// Check if the counter is wrapping
		if (seq == UINT32_MAX)
			seq = 0;
		else
			seq++;
	}

	return NULL;
}

void *tunnel_read_socket_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	struct mmsghdr *msgs;
	struct iovec *iovecs;
	struct sockaddr_in6 *controls;
	ssize_t sockaddr_len = sizeof(struct sockaddr_in6);
	BufferNode **buffer_nodes;
	PacketHeader *pkthdr;

	// Allocate memory for our buffers and IO vectors
	buffer_nodes = (BufferNode **)malloc(STAP_MAX_RECVMM_MESSAGES * sizeof(BufferNode));
	msgs = (struct mmsghdr *)malloc(STAP_MAX_RECVMM_MESSAGES * sizeof(struct mmsghdr));
	iovecs = (struct iovec *)malloc(STAP_MAX_RECVMM_MESSAGES * sizeof(struct iovec));
	controls = (struct sockaddr_in6 *)calloc(1, STAP_MAX_RECVMM_MESSAGES * sizeof(struct sockaddr_in6));
	// Make sure memory allocation succeeded
	if (msgs == NULL || iovecs == NULL || controls == NULL || buffer_nodes == NULL) {
		perror("tunnel_read_socket_handler(): Memory allocation failed");
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
			perror("tunnel_read_socket_handler(): Could not get available buffer node");
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


	while (1) {
		if (*tdata->stop_program) break;

		int num_received = recvmmsg(tdata->udp_socket, msgs, STAP_MAX_RECVMM_MESSAGES, MSG_WAITFORONE, NULL);
		// fprintf(stderr, "Read something\n");
		if (num_received == -1) {
			perror("tunnel_read_socket_handler(): recvmmsg failed");
			break;
		}

		for (int i = 0; i < num_received; ++i) {
			char ipv6_str[INET6_ADDRSTRLEN];
			if (inet_ntop(AF_INET6, &controls[i].sin6_addr, ipv6_str, sizeof(ipv6_str)) == NULL) {
				perror("inet_ntop failed");
				exit(EXIT_FAILURE);
			}
			DEBUG_PRINT("tunnel_read_socket_handler(): Received %u bytes from %s:%d with flags %i\n", msgs[i].msg_len, ipv6_str,
						ntohs(controls[i].sin6_port), msgs[i].msg_hdr.msg_flags);
			// Compare to see if this is the remote system IP
			if (memcmp(&tdata->remote_addr.sin6_addr, &controls[i].sin6_addr, sizeof(struct in6_addr))) {
				fprintf(stderr, "tunnel_read_socket_handler(): Source address is incorrect %s!", ipv6_str);
				continue;
			}

			// Grab this IOV's buffer node and update the actual buffer length
			BufferNode *buffer_node = buffer_nodes[i];
			buffer_node->buffer->length = msgs[i].msg_len;

			// Overlay packet header and do basic header checks
			pkthdr = (PacketHeader *)buffer_node->buffer->contents;
			if (pkthdr->opt_len != 0) {
				fprintf(stderr, "tunnel_read_socket_handler(): Packet should not have options, it has %u, DROPPING!",
						pkthdr->opt_len);
				continue;
			}
			if (pkthdr->reserved != 0) {
				fprintf(stderr, "tunnel_read_socket_handler(): Packet should not have any reserved its set, it is %u, DROPPING!",
						pkthdr->opt_len);
				continue;
			}
			// First thing we do is validate the header
			if (pkthdr->format != STAP_PACKET_FORMAT_ENCAP) {
				fprintf(stderr, "tunnel_read_socket_handler(): Packet in invalid format %x, DROPPING!", pkthdr->format);
				continue;
			}
			// Next check the channel is set to 1
			if (pkthdr->channel != 1) {
				fprintf(stderr, "tunnel_read_socket_handler(): Packet specifies invalid channel %d, DROPPING!", pkthdr->channel);
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

void *tunnel_write_tap_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;
	ssize_t max_queue_size = 0;
	PacketHeader *pkthdr;
	int first_packet = 1;
	uint32_t seq = 0;
	uint32_t last_seq = 0;

	while (1) {
		if (*tdata->stop_program) break;

		// Grab buffer
		BufferNode *buffer_node;
		int result = get_buffer_node_head_wait(&tdata->rx_packet_queue, &buffer_node, 0, 0);
		// Grab buffer list size
		int cur_queue_size = get_buffer_list_size(&tdata->rx_packet_queue);
		if (cur_queue_size > max_queue_size) {
			max_queue_size = cur_queue_size;
			fprintf(stderr, "tunnel_write_tap_handler(): Max RX packet queue size %li\n", max_queue_size);
		}
		// Check we actually got something
		if (result != 0) {
			fprintf(stderr, "tunnel_write_tap_handler(): pthread_cond_wait() seems to of failed?");
			exit(EXIT_FAILURE);
		}
		if (!buffer_node) {
			fprintf(stderr, "tunnel_write_tap_handler(): Failed to get buffer, list probably empty");
			exit(EXIT_FAILURE);
		}
		// Set the buffer variable so we can access it more easily
		Buffer *buffer = buffer_node->buffer;

		// Set up packet header, we point it to the buffer
		pkthdr = (PacketHeader *)buffer->contents;

		// Grab sequence
		seq = stap_be_to_cpu_32(pkthdr->sequence);
		// If this is not the first packet...
		if (!first_packet) {
			// Check if we've lost packets
			if (seq > last_seq + 1) {
				fprintf(stderr, "tunnel_read_socket_handler(): PACKET LOST: last=%u, seq=%u, total_lost=%u\n", last_seq, seq,
						seq - last_seq);
			}
			// If we we have an out of order one
			if (seq < last_seq + 1) {
				fprintf(stderr, "tunnel_read_socket_handler(): PACKET OOO : last=%u, seq=%u\n", last_seq, seq);
			}
		}

		// Write data to TAP interface
		ssize_t wbytes = write(tdata->tap_device.fd, buffer->contents + STAP_PACKET_HEADER_SIZE, buffer->length - STAP_PACKET_HEADER_SIZE);
		if (wbytes == -1) {
			perror("tunnel_read_socket_handler(): Error writing TAP device");
			exit(EXIT_FAILURE);
		}
		DEBUG_PRINT("%s: write %li bytes to TAP\n", __func__, wbytes);

		// Add buffer back to available buffer list
		append_buffer_node(&tdata->available_buffers, buffer_node);

		// If this was the first packet, reset the first packet indicator
		if (first_packet) {
			first_packet = 0;
			last_seq = seq;
			// We only bump the last sequence if its smaller than the one we're on
		} else if (last_seq < seq) {
			last_seq = seq;
		} else if (last_seq > seq && sequence_wrapping(seq, last_seq)) {
			last_seq = seq;
		}
	}

	return NULL;
}
