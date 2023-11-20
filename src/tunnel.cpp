/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "tunnel.hpp"
#include "Decoder.hpp"
#include "Encoder.hpp"
#include "Endian.hpp"
#include "PacketBuffer.hpp"
#include "common.hpp"
#include "libaccl/BufferPool.hpp"
#include "libaccl/Logger.hpp"
#include "tap.hpp"
#include "threads.hpp"
#include "util.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <set>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

/**
 * @brief Thread responsible for reading from the TAP interface.
 *
 * @param arg Thread data.
 */
void tunnel_tap_read_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	LOG_DEBUG_INTERNAL("TAP READ: Starting TAP read thread");

	// Loop and read data from TAP device
	while (true) {
		// Check for program stop
		if (*tdata->stop_program) {
			break;
		}

		// Grab a new buffer for our data
		LOG_DEBUG_INTERNAL("AVAIL POOL: Buffer pool count: ", tdata->rx_buffer_pool->getBufferCount(), ", taking one");
		auto buffer = tdata->rx_buffer_pool->pop();

		// Read data from TAP interface
		ssize_t bytes_read = read(tdata->tap_device.fd, buffer->getData(), buffer->getBufferSize());
		if (bytes_read == -1) {
			LOG_ERROR("Got an error read()'ing TAP device: ", strerror(errno));
			exit(EXIT_FAILURE);
		} else if (bytes_read == 0) {
			LOG_ERROR("Got EOF from TAP device: ", strerror(errno));
			exit(EXIT_FAILURE);
		}

		LOG_DEBUG_INTERNAL("TAP READ: Read ", bytes_read, " bytes from TAP");
		buffer->setDataSize(bytes_read);

		// Queue packet to encode
		tdata->encoder_pool->push(std::move(buffer));
	}
}

/**
 * @brief Thread responsible for encoding packets.
 *
 * @param arg Thread data.
 */
void tunnel_encoder_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	LOG_DEBUG_INTERNAL("ENCODER: Starting encoder thread");

	// Packet encoder
	PacketEncoder encoder(tdata->l2mtu, tdata->l4mtu, tdata->rx_buffer_pool, tdata->socket_write_pool);

	// Loop pulling buffers off the encoder pool
	std::chrono::milliseconds timeout = std::chrono::milliseconds(1);
	std::deque<std::unique_ptr<PacketBuffer>> buffers;
	while (true) {
		// Check for program stop
		if (*tdata->stop_program) {
			break;
		}

		// Check if we timed out
		if (!tdata->encoder_pool->wait_for(timeout, buffers)) {
			// If we did, flush the encoder and zero the wait so we wait indefinitely
			encoder.flush();
			timeout = std::chrono::milliseconds::zero();
			continue;
		}

		// Loop with the buffers we got
		for (auto &buffer : buffers) {
			encoder.encode(std::move(buffer));
		}
		// Clear buffers
		buffers.clear();

		// Set timeout to 1ms
		timeout = std::chrono::milliseconds(1);
	}
}

/**
 * @brief Thread responsible for writing encoded packets to the socket.
 *
 * @param arg Thread data.
 */
void tunnel_socket_write_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	LOG_DEBUG_INTERNAL("SOCKET WRITE: Starting socket write thread");

	// Constant used below
	socklen_t addrlen = sizeof(struct sockaddr_in6);
	// sendto() flags
	int flags = 0;

	// Loop pulling buffers off the socket write pool
	std::deque<std::unique_ptr<PacketBuffer>> buffers;
	while (true) {
		// Check for program stop
		if (*tdata->stop_program) {
			break;
		}

		// Wait for buffers
		tdata->socket_write_pool->wait(buffers);

		// Loop with the buffers we got
#ifdef DEBUG
		size_t i{0};
#endif
		for (auto &buffer : buffers) {
			// Write data out
			ssize_t bytes_written = sendto(tdata->udp_socket, buffer->getData(), buffer->getDataSize(), flags,
										   (struct sockaddr *)&tdata->remote_addr, addrlen);
			if (bytes_written == -1) {
				LOG_ERROR("Got an error in sendto(): ", strerror(errno));
				exit(EXIT_FAILURE);
			}
#ifdef DEBUG
			i++;
			LOG_DEBUG_INTERNAL("Wrote ", bytes_written, " bytes to tunnel [buffer: ", i, "/", buffers.size(), "]");
#endif
		}

		// Push buffers into available pool
		tdata->rx_buffer_pool->push(buffers);
	}
}

/**
 * @brief Internal function to compare a packet
 *
 */
struct _comparePacketBuffer {
		bool operator()(const std::unique_ptr<PacketBuffer> &a, const std::unique_ptr<PacketBuffer> &b) const {
			// Use our own comparison operator inside PacketBuffer
			return a->getKey() < b->getKey();
		}
};

/**
 * @brief Thread responsible for reading data from the socket.
 *
 * @param arg Thread data.
 */
void tunnel_socket_read_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	struct sockaddr_in6 *controls;
	ssize_t sockaddr_len = sizeof(struct sockaddr_in6);
	std::vector<std::unique_ptr<PacketBuffer>> recvmm_buffers(SETH_MAX_RECVMM_MESSAGES);
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
		LOG_ERROR("Memory allocation failed for recvmm iovecs: ", strerror(errno));
		// Clean up previously allocated memory
		free(msgs);
		free(iovecs);
		free(controls);
		exit(EXIT_FAILURE);
	}

	// Set up iovecs and mmsghdrs
	for (size_t i = 0; i < SETH_MAX_RECVMM_MESSAGES; ++i) {
		recvmm_buffers[i] = tdata->tx_buffer_pool->pop();

		// Setup IO vectors
		iovecs[i].iov_base = recvmm_buffers[i]->getData();
		iovecs[i].iov_len = recvmm_buffers[i]->getBufferSize();
		// And the associated msgs
		msgs[i].msg_hdr.msg_iov = &iovecs[i];
		msgs[i].msg_hdr.msg_iovlen = 1;
		msgs[i].msg_hdr.msg_name = (void *)&controls[i];
		msgs[i].msg_hdr.msg_namelen = sockaddr_len;
	}
	// END - IO vector buffers

	// Received buffers
	// std::set<std::unique_ptr<PacketBuffer>, _comparePacketBuffer> received_buffers;
	std::deque<std::unique_ptr<PacketBuffer>> received_buffers;
	while (1) {
		if (*tdata->stop_program)
			break;

		int num_received = recvmmsg(tdata->udp_socket, msgs, SETH_MAX_RECVMM_MESSAGES, MSG_WAITFORONE, NULL);
		if (num_received == -1) {
			LOG_ERROR("recvmmsg failed: ", strerror(errno));
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < num_received; ++i) {
			char ipv6_str[INET6_ADDRSTRLEN];
			if (inet_ntop(AF_INET6, &controls[i].sin6_addr, ipv6_str, sizeof(ipv6_str)) == NULL) {
				LOG_ERROR("inet_ntop failed: ", strerror(errno));
				exit(EXIT_FAILURE);
			}
			LOG_DEBUG_INTERNAL("Received ", msgs[i].msg_len, " bytes from ", ipv6_str, ":", ntohs(controls[i].sin6_port),
							   " with flags ", msgs[i].msg_hdr.msg_flags);
			// Compare to see if this is the remote system IP
			if (memcmp(&tdata->remote_addr.sin6_addr, &controls[i].sin6_addr, sizeof(struct in6_addr))) {
				LOG_ERROR("Source address is incorrect: ", ipv6_str);
				continue;
			}

			// Grab this IOV's buffer node and update the actual buffer length
			recvmm_buffers[i]->setDataSize(msgs[i].msg_len);

			// Make sure the buffer is long enough for us to overlay the header
			if (recvmm_buffers[i]->getDataSize() < sizeof(PacketHeader) + sizeof(PacketHeaderOption)) {
				LOG_ERROR("Packet too small ", recvmm_buffers[i]->getDataSize(), " < ",
						  sizeof(PacketHeader) + sizeof(PacketHeaderOption), ", DROPPING!!!");
				continue;
			}

			// Overlay packet header and do basic header checks
			pkthdr = (PacketHeader *)recvmm_buffers[i]->getData();
			// Check version is supported
			if (pkthdr->ver > SETH_PACKET_HEADER_VERSION_V1) {
				LOG_ERROR("Packet not supported, version ", static_cast<uint8_t>(pkthdr->ver), " vs. our version ",
						  SETH_PACKET_HEADER_VERSION_V1, ", DROPPING!");
				continue;
			}
			if (pkthdr->reserved != 0) {
				LOG_ERROR("Packet header should not have any reserved its set, it is ", static_cast<uint8_t>(pkthdr->reserved),
						  ", DROPPING!");
				continue;
			}
			// First thing we do is validate the header
			int valid_format_mask =
				static_cast<uint8_t>(PacketHeaderFormat::ENCAPSULATED) | static_cast<uint8_t>(PacketHeaderFormat::COMPRESSED);
			if (static_cast<uint8_t>(pkthdr->format) & ~valid_format_mask) {
				LOG_ERROR("Packet in invalid format ", static_cast<uint8_t>(pkthdr->format), ", DROPPING!");
				continue;
			}
			// Next check the channel is set to 0
			if (pkthdr->channel) {
				LOG_ERROR("Packet specifies invalid channel ", pkthdr->channel, ", DROPPING!");
				continue;
			}

			// Add buffer node to the received list
			recvmm_buffers[i]->setKey(seth_be_to_cpu_32(pkthdr->sequence));
			// received_buffers.insert(std::move(recvmm_buffers[i]));
			received_buffers.push_back(std::move(recvmm_buffers[i]));

			// Replenish the buffer
			recvmm_buffers[i] = tdata->tx_buffer_pool->pop();
			msgs[i].msg_hdr.msg_iov->iov_base = recvmm_buffers[i]->getData();
		}
		// Push all the buffers we got
		tdata->decoder_pool->push(received_buffers);
		received_buffers.clear();
	}
	// Free the rest of the IOV stuff
	free(msgs);
	free(iovecs);
}

/**
 * @brief Thread responsible for decoding packets.
 *
 * @param arg Thread data.
 */
void tunnel_decoder_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	LOG_DEBUG_INTERNAL("DECODER: Starting decoder thread");

	PacketDecoder decoder(tdata->l2mtu, tdata->tx_buffer_pool, tdata->tap_write_pool);

	// Loop pulling buffers off the socket write pool
	std::deque<std::unique_ptr<PacketBuffer>> buffers;
	while (true) {
		// Check for program stop
		if (*tdata->stop_program) {
			break;
		}

		// Wait for buffers
		tdata->decoder_pool->wait(buffers);

		// Loop with buffers
		for (auto &buffer : buffers) {
			decoder.decode(std::move(buffer));
		}
		buffers.clear();
	}
}

/**
 * @brief Thread responsible for writing to the TAP device.
 *
 * @param arg Thread data.
 */
void tunnel_tap_write_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	// Loop pulling buffers off the socket write pool
	std::deque<std::unique_ptr<PacketBuffer>> buffers;
	while (true) {
		// Check for program stop
		if (*tdata->stop_program) {
			break;
		}

		// Wait for buffers
		tdata->tap_write_pool->wait(buffers);

		// Loop with buffers
#ifdef DEBUG
		size_t i{0};
#endif
		for (auto &buffer : buffers) {

			// Write data to TAP interface
			ssize_t bytes_written = write(tdata->tap_device.fd, buffer->getData(), buffer->getDataSize());
			if (bytes_written == -1) {
				LOG_ERROR("Error writing TAP device: ", strerror(errno));
				exit(EXIT_FAILURE);
			}
#ifdef DEBUG
			i++;
			LOG_DEBUG_INTERNAL("Wrote ", bytes_written, " bytes to TAP [", i, "/", buffers.size(), "]");
#endif
		}
		// Push buffers into available pool
		tdata->tx_buffer_pool->push(buffers);
		buffers.clear();
	}
}