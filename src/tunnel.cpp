/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "tunnel.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <string.h>

#include "Codec.hpp"
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

		// Loop with buffers and send to the remote end
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

// Read data from socket and queue to the decoder
void tunnel_read_socket_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	struct sockaddr_in6 *controls;
	ssize_t sockaddr_len = sizeof(struct sockaddr_in6);
	std::vector<std::unique_ptr<accl::Buffer>> buffers(SETH_MAX_RECVMM_MESSAGES);
	struct mmsghdr *msgs;
	struct iovec *iovecs;
	PacketHeader *pkthdr;

	// START - IO vector buffers

	// Allocate memory for our IO vectors
	msgs = (struct mmsghdr *)malloc(SETH_MAX_RECVMM_MESSAGES * sizeof(struct mmsghdr));
	iovecs = (struct iovec *)malloc(SETH_MAX_RECVMM_MESSAGES * sizeof(struct iovec));
	controls = (struct sockaddr_in6 *)calloc(1, SETH_MAX_RECVMM_MESSAGES * sizeof(struct sockaddr_in6));

	// Make sure memory allocation succeeded
	if (msgs == NULL || iovecs == NULL || controls == NULL) {
		CERR("Memory allocation failed: {}", strerror(errno));
		// Clean up previously allocated memory
		free(msgs);
		free(iovecs);
		free(controls);
		exit(EXIT_FAILURE);
	}

	// Set up iovecs and mmsghdrs
	for (size_t i = 0; i < SETH_MAX_RECVMM_MESSAGES; ++i) {
		buffers[i] = tdata->available_buffer_pool->pop();

		// Setup IO vectors
		iovecs[i].iov_base = buffers[i]->getData();
		iovecs[i].iov_len = buffers[i]->getBufferSize();
		// And the associated msgs
		msgs[i].msg_hdr.msg_iov = &iovecs[i];
		msgs[i].msg_hdr.msg_iovlen = 1;
		msgs[i].msg_hdr.msg_name = (void *)&controls[i];
		msgs[i].msg_hdr.msg_namelen = sockaddr_len;
	}
	// END - IO vector buffers

	while (1) {
		if (*tdata->stop_program)
			break;

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
			buffers[i]->setDataSize(msgs[i].msg_len);

			// Make sure the buffer is long enough for us to overlay the header
			if (buffers[i]->getDataSize() < sizeof(PacketHeader) + sizeof(PacketHeaderOption)) {
				CERR("Packet too small {} < {}, DROPPING!!!", buffers[i]->getDataSize(),
					 sizeof(PacketHeader) + sizeof(PacketHeaderOption));
				continue;
			}

			// Overlay packet header and do basic header checks
			pkthdr = (PacketHeader *)buffers[i]->getData();
			// Check version is supported
			if (pkthdr->ver > SETH_PACKET_HEADER_VERSION_V1) {
				CERR("Packet not supported, version {} vs. our version {}, DROPPING!", static_cast<uint8_t>(pkthdr->ver),
					 SETH_PACKET_HEADER_VERSION_V1);
				continue;
			}
			if (pkthdr->reserved != 0) {
				CERR("Packet header should not have any reserved its set, it is {}, DROPPING!",
					 static_cast<uint8_t>(pkthdr->reserved));
				continue;
			}
			// First thing we do is validate the header
			int valid_format_mask =
				static_cast<uint8_t>(PacketHeaderFormat::ENCAPSULATED) | static_cast<uint8_t>(PacketHeaderFormat::COMPRESSED);
			if (static_cast<uint8_t>(pkthdr->format) & ~valid_format_mask) {
				CERR("Packet in invalid format {}, DROPPING!", static_cast<uint8_t>(pkthdr->format));
				continue;
			}
			// Next check the channel is set to 0
			if (pkthdr->channel) {
				CERR("Packet specifies invalid channel {}, DROPPING!", pkthdr->channel);
				continue;
			}

			// Add buffer node to RX queue
			tdata->decoder_buffer_pool->push(buffers[i]);

			// Replenish the buffer
			buffers[i] = tdata->available_buffer_pool->pop();
			msgs[i].msg_hdr.msg_iov->iov_base = buffers[i]->getData();
		}
	}

	// Add buffers back to available list for de-allocation
	for (uint32_t i = 0; i < SETH_MAX_RECVMM_MESSAGES; ++i) {
		tdata->available_buffer_pool->push(buffers[i]);
	}
	// Free the rest of the IOV stuff
	free(msgs);
	free(iovecs);
}

// Decode packets arriving on the decoder buffer pool and dump them into the rx buffer pool
void tunnel_decoding_handler(void *arg) {
	// Thread data
	struct ThreadData *tdata = (struct ThreadData *)arg;

	// Packet decoder
	PacketDecoder decoder(tdata->mtu, tdata->available_buffer_pool, tdata->rx_buffer_pool);

	while (1) {
		if (*tdata->stop_program)
			break;

		// Grab buffer
		std::vector<std::unique_ptr<accl::Buffer>> buffers = tdata->decoder_buffer_pool->wait();

		// Loop with buffers and decode
		for (auto &buffer : buffers) {
			decoder.decode(*buffer);
			// Push buffer back to pool
			tdata->available_buffer_pool->push(buffer);
		}
	}
}

// Write data from the decoder buffer pool to the TAP device
void tunnel_write_tap_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;
	ssize_t max_queue_size = 0;

	while (1) {
		if (*tdata->stop_program)
			break;

		// Grab buffer
		std::vector<std::unique_ptr<accl::Buffer>> buffers = tdata->decoder_buffer_pool->wait();

		// Grab buffer list size
		int cur_queue_size = buffers.size();
		if (cur_queue_size > max_queue_size) {
			max_queue_size = cur_queue_size;
			CERR("Max RX buffer pool size {}", max_queue_size);
		}

		// Loop with buffers and decode
		size_t i{0};
		for (auto &buffer : buffers) {
			// Write data to TAP interface
			ssize_t bytes_written = write(tdata->tap_device.fd, buffer->getData(), buffer->getDataSize());
			if (bytes_written == -1) {
				CERR("Error writing TAP device: {}", strerror(errno));
				exit(EXIT_FAILURE);
			}
			DEBUG_CERR("Wrote {} bytes to TAP [{}/{}]", bytes_written, i + 1, buffers.size());

			// Push buffer back to pool
			tdata->available_buffer_pool->push(buffer);
		}
	}
}
