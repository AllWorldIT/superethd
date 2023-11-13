/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "tunnel.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <string.h>

#include "Codec.hpp"
#include "buffers.hpp"
#include "common.hpp"
#include "debug.hpp"
#include "libaccl/Buffer.hpp"
#include "libaccl/BufferPool.hpp"
#include "tap.hpp"
#include "threads.hpp"
#include "util.hpp"

extern "C" {
#include <lz4.h>
}

// Reads data from the TAP interface and queues packets for sending over the tunnel.
void tunnel_read_tap_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	while (1) {
		if (*tdata->stop_program)
			break;

		// Grab a new buffer for our data
		auto buffer = tdata->available_buffer_pool->pop();

		// Read data from TAP interface
		ssize_t bytes_read = read(tdata->tap_device.fd, buffer->getData(), buffer->getBufferSize());
		if (bytes_read == -1) {
			CERR("Got an error read()'ing TAP device: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		DEBUG_CERR("AVAIL Buffer pool count: {}", tdata->available_buffer_pool->getBufferCount());
		DEBUG_CERR("TXQUEUE Buffer pool count: {}", tdata->encoder_buffer_pool->getBufferCount());

		DEBUG_CERR("Read {} bytes from TAP", bytes_read);
		buffer->setDataSize(bytes_read);

		// Append buffer node to queue
		tdata->encoder_buffer_pool->push(buffer);
	}
}

// Read data from the encoder buffer pool and write to the tx buffer pool.
void tunnel_encoding_handler(void *arg) {
	// Thread data
	struct ThreadData *tdata = (struct ThreadData *)arg;
	// Just a stat
	ssize_t max_queue_size = 0;

	// Packet encoder
	PacketEncoder encoder(tdata->tx_size, tdata->mtu, tdata->available_buffer_pool, tdata->tx_buffer_pool);

	// Main IO loop
	std::chrono::milliseconds buffer_wait_time{0};
	std::vector<std::unique_ptr<accl::Buffer>> buffers;
	while (1) {
		if (*tdata->stop_program)
			break;

		// Grab buffer
		bool res = tdata->encoder_buffer_pool->wait_for(buffer_wait_time, buffers);
		if (!res) {
			DEBUG_CERR("buffer wait timeout triggered!");
			encoder.flushBuffer();
			buffer_wait_time = std::chrono::milliseconds{0};
			continue;
		}

		// Grab buffer list size
		int cur_queue_size = buffers.size();
		if (cur_queue_size > max_queue_size) {
			max_queue_size = cur_queue_size;
			CERR("Max ENCODER packet queue size {}", max_queue_size);
		}

		// Loop with buffers and encode
		for (auto &buffer : buffers) {
			encoder.encode(*buffer);
			tdata->available_buffer_pool->push(buffer);
		}

		// Set sleep to 1ms
		buffer_wait_time = std::chrono::milliseconds(1);

		// Clear buffer list
		buffers.clear();
	}
}

// Write data from the encoded buffer pool to the socket.
void tunnel_write_socket_handler(void *arg) {
	// Thread data
	struct ThreadData *tdata = (struct ThreadData *)arg;
	// Constant used below
	socklen_t addrlen = sizeof(struct sockaddr_in6);

	// sendto() flags
	int flags = 0;

	while (1) {
		if (*tdata->stop_program)
			break;

		// Grab buffer
		std::vector<std::unique_ptr<accl::Buffer>> buffers = tdata->tx_buffer_pool->wait();

		// Loop with buffers and encode
		size_t i{0};
		for (auto &buffer : buffers) {
			// Write data out
			ssize_t bytes_written = sendto(tdata->udp_socket, buffer->getData(), buffer->getDataSize(), flags,
										   (struct sockaddr *)&tdata->remote_addr, addrlen);
			if (bytes_written == -1) {
				CERR("Got an error on sendto(): {}", strerror(errno));
				exit(EXIT_FAILURE);
			}
			DEBUG_CERR("Wrote {} bytes to tunnel [{}/{}]", bytes_written, i + 1, buffers.size());
			// Push buffer back to pool
			tdata->available_buffer_pool->push(buffer);
			i++;
		}
	}
}

#if 0
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
		CERR("Memory allocation failed: {}", strerror(errno));
		// Clean up previously allocated memory
		free(buffer_nodes);
		free(msgs);
		free(iovecs);
		free(controls);
		exit(EXIT_FAILURE);
	}
	// Set up iovecs and mmsghdrs
	for (size_t i = 0; i < SETH_MAX_RECVMM_MESSAGES; ++i) {
		buffer_nodes[i] = get_buffer_node_head(&tdata->available_buffers);
		if (buffer_nodes[i] == NULL) {
			CERR("Could not get available buffer node");
			// Clean up allocated memory
			for (size_t j = 0; j < i; ++j) {
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
			CERR("recvmmsg failed: {}", strerror(errno));
			break;
		}

		for (int i = 0; i < num_received; ++i) {
			char ipv6_str[INET6_ADDRSTRLEN];
			if (inet_ntop(AF_INET6, &controls[i].sin6_addr, ipv6_str, sizeof(ipv6_str)) == NULL) {
				CERR("inet_ntop failed: {}", strerror(errno));
				exit(EXIT_FAILURE);
			}
			DEBUG_CERR("Received {} bytes from {}:{} with flags {}", msgs[i].msg_len, ipv6_str, ntohs(controls[i].sin6_port),
					   msgs[i].msg_hdr.msg_flags);
			// Compare to see if this is the remote system IP
			if (memcmp(&tdata->remote_addr.sin6_addr, &controls[i].sin6_addr, sizeof(struct in6_addr))) {
				CERR("Source address is incorrect {}!", ipv6_str);
				continue;
			}

			// Grab this IOV's buffer node and update the actual buffer length
			BufferNode *buffer_node = buffer_nodes[i];
			buffer_node->buffer->length = msgs[i].msg_len;

			// Make sure the buffer is long enough for us to overlay the header
			if (buffer_node->buffer->length < SETH_PACKET_HEADER_SIZE) {
				CERR("Packet too small {} < 12, DROPPING!!!", buffer_node->buffer->length);
				continue;
			}

			// Overlay packet header and do basic header checks
			pkthdr = (PacketHeader *)buffer_node->buffer->contents;
			// Check version is supported
			if (pkthdr->ver > SETH_PACKET_VERSION_V1) {
				CERR("Packet not supported, version {} vs. our version {}, DROPPING!", static_cast<uint8_t>(pkthdr->ver), SETH_PACKET_VERSION_V1);
				continue;
			}
			if (pkthdr->reserved != 0) {
				CERR("Packet header should not have any reserved its set, it is {}, DROPPING!", static_cast<uint8_t>(pkthdr->reserved));
				continue;
			}
			// First thing we do is validate the header
			int valid_format_mask = SETH_PACKET_FORMAT_ENCAP | SETH_PACKET_FORMAT_COMPRESSED;
			if (pkthdr->format & ~valid_format_mask) {
				CERR("Packet in invalid format {}, DROPPING!", pkthdr->format);
				continue;
			}
			// Next check the channel is set to 0
			if (pkthdr->channel) {
				CERR("Packet specifies invalid channel {}, DROPPING!", pkthdr->channel);
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
	for (uint32_t i = 0; i < SETH_MAX_RECVMM_MESSAGES; ++i) {
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
	DEBUG_CERR("Allocating {} wbuffers of {} size each", wbuffer_count, tdata->max_ethernet_frame_size);
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
			CERR("pthread_cond_wait() seems to of failed?");
			exit(EXIT_FAILURE);
		}
		if (!buffer_node) {
			CERR("Failed to get buffer, list probably empty");
			exit(EXIT_FAILURE);
		}

		// Grab buffer list size
		int cur_queue_size = get_buffer_list_size(&tdata->rx_packet_queue);
		if (cur_queue_size > max_queue_size) {
			max_queue_size = cur_queue_size;
			CERR("Max RX packet queue size {}", max_queue_size);
		}

		// Encode packet and get number of resulting packets generated, these are stored in our wbuffers
		int pktcount = packet_decoder(&state, &wbuffers, buffer_node->buffer, tdata->mtu);
		DEBUG_CERR("Got %i packets back from packet decoder", pktcount);
		// Add buffer back to available list
		append_buffer_node(&tdata->available_buffers, buffer_node);

		// Loop with our buffers and write them out
		BufferNode *wbuffer_node = wbuffers.head;
		for (int i = 0; i < pktcount; ++i) {
			// Write data to TAP interface
			ssize_t bytes_written = write(tdata->tap_device.fd, wbuffer_node->buffer->contents, wbuffer_node->buffer->length);
			if (bytes_written == -1) {
				CERR("Error writing TAP device: {}", strerror(errno));
				exit(EXIT_FAILURE);
			}
			DEBUG_CERR("Wrote {} bytes to TAP [{}/{}]", bytes_written, i + 1, pktcount);
			// Go to next buffer
			wbuffer_node = wbuffer_node->next;
		}
	}

	return NULL;
}
#endif
