/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "tunnel.hpp"
#include "Codec.hpp"
#include "common.hpp"
#include "libaccl/Buffer.hpp"
#include "libaccl/BufferPool.hpp"
#include "libaccl/Logger.hpp"
#include "tap.hpp"
#include "threads.hpp"
#include "util.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

void tunnel_read_tap_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	// Packet encoder
	auto buffer_pool = new accl::BufferPool(tdata->l2mtu, SETH_BUFFER_COUNT);
	auto encoder_pool = new accl::BufferPool(tdata->l2mtu);
	PacketEncoder encoder(tdata->l2mtu, tdata->l4mtu, buffer_pool, encoder_pool);

	// Grab epoll FD
	int epollFd = epoll_create1(0);
	if (epollFd == -1) {
		std::cerr << "Failed to create epoll file descriptor: " << strerror(errno) << std::endl;
		return;
	}

	// Setup epoll events
	struct epoll_event ev;
	ev.events = EPOLLIN; // Interested in input (read) events
	ev.data.fd = tdata->tap_device.fd;

	// Add TAP device FD
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, tdata->tap_device.fd, &ev) == -1) {
		LOG_ERROR("Failed to add fd to epoll: {}", strerror(errno));
		close(epollFd);
		return;
	}

	// Constant used below
	socklen_t addrlen = sizeof(struct sockaddr_in6);
	// sendto() flags
	int flags = 0;

	// Carry on looping
	int timeout = -1;
	while (true) {
		// Check for program stop
		if (*tdata->stop_program) {
			break;
		}

		// Wait for something to become available
		struct epoll_event events[1];
		int nrEvents = epoll_wait(epollFd, events, 1, timeout);
		if (nrEvents == 0) { // timeout
			encoder.flush();
			timeout = -1;
			goto write_socket;

		} else if (nrEvents == -1) {
			std::cerr << "Epoll wait error: " << strerror(errno) << std::endl;
			break;
		}

		// NK: We enclose the read section in braces or we bypass the variable initialization
		{
			// Grab a new buffer for our data
			LOG_DEBUG_INTERNAL("AVAIL POOL: Buffer pool count: {}, taking one", buffer_pool->getBufferCount());
			auto buffer = buffer_pool->pop();
			
			// Read data from TAP interface
			ssize_t bytes_read = read(tdata->tap_device.fd, buffer->getData(), buffer->getBufferSize());
			if (bytes_read == -1) {
				LOG_ERROR("Got an error read()'ing TAP device: %s", strerror(errno));
				exit(EXIT_FAILURE);
			} else if (bytes_read == 0) {
				LOG_ERROR("Got EOF from TAP device: %s", strerror(errno));
				exit(EXIT_FAILURE);
			}

			LOG_DEBUG_INTERNAL("TAP READ: Read {} bytes from TAP", bytes_read);
			buffer->setDataSize(bytes_read);

			// Encode packet
			encoder.encode(std::move(buffer));
			timeout = 1;
		}

	write_socket:
		// Next check if there is anything that was encoded
		if (!encoder_pool->getBufferCount()) {
			continue;
		}

		size_t i{0};
		auto buffers = encoder_pool->pop(0);
		for (auto &b : buffers) {
			// Write data out
			ssize_t bytes_written = sendto(tdata->udp_socket, b->getData(), b->getDataSize(), flags,
										   (struct sockaddr *)&tdata->remote_addr, addrlen);
			if (bytes_written == -1) {
				LOG_ERROR("Got an error in sendto(): {}", strerror(errno));
				exit(EXIT_FAILURE);
			}

			i++;
			LOG_DEBUG_INTERNAL("Wrote {} bytes to tunnel [{}/{}]", bytes_written, i, buffers.size());
		}

		// Push buffers into available pool
		LOG_DEBUG_INTERNAL("Buffer pool size: {}", buffer_pool->getBufferCount());
		buffer_pool->push(buffers);
		LOG_DEBUG_INTERNAL("Buffer pool size after push: {}", buffer_pool->getBufferCount());
	}

	close(epollFd);
}

// Read data from socket and queue to the decoder
void tunnel_read_socket_handler(void *arg) {
	struct ThreadData *tdata = (struct ThreadData *)arg;

	// Packet decoder
	auto buffer_pool = new accl::BufferPool(tdata->l2mtu, SETH_BUFFER_COUNT);
	auto decoder_pool = new accl::BufferPool(tdata->l2mtu);
	PacketDecoder decoder(tdata->l2mtu, buffer_pool, decoder_pool);

	struct sockaddr_in6 *controls;
	ssize_t sockaddr_len = sizeof(struct sockaddr_in6);
	std::vector<std::unique_ptr<accl::Buffer>> recvmm_buffers(SETH_MAX_RECVMM_MESSAGES);
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
		LOG_ERROR("Memory allocation failed for recvmm iovecs: {}", strerror(errno));
		// Clean up previously allocated memory
		free(msgs);
		free(iovecs);
		free(controls);
		exit(EXIT_FAILURE);
	}

	// Set up iovecs and mmsghdrs
	for (size_t i = 0; i < SETH_MAX_RECVMM_MESSAGES; ++i) {
		recvmm_buffers[i] = buffer_pool->pop();

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


	while (1) {
		if (*tdata->stop_program)
			break;

		int num_received = recvmmsg(tdata->udp_socket, msgs, SETH_MAX_RECVMM_MESSAGES, MSG_WAITFORONE, NULL);
		if (num_received == -1) {
			LOG_ERROR("recvmmsg failed: {}", strerror(errno));
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < num_received; ++i) {
			char ipv6_str[INET6_ADDRSTRLEN];
			if (inet_ntop(AF_INET6, &controls[i].sin6_addr, ipv6_str, sizeof(ipv6_str)) == NULL) {
				LOG_ERROR("inet_ntop failed: {}", strerror(errno));
				exit(EXIT_FAILURE);
			}
			LOG_DEBUG_INTERNAL("Received {} bytes from {}:{} with flags {}", msgs[i].msg_len, ipv6_str,
							   ntohs(controls[i].sin6_port), msgs[i].msg_hdr.msg_flags);
			// Compare to see if this is the remote system IP
			if (memcmp(&tdata->remote_addr.sin6_addr, &controls[i].sin6_addr, sizeof(struct in6_addr))) {
				LOG_ERROR("Source address is incorrect {}!", ipv6_str);
				continue;
			}

			// Grab this IOV's buffer node and update the actual buffer length
			recvmm_buffers[i]->setDataSize(msgs[i].msg_len);

			// Make sure the buffer is long enough for us to overlay the header
			if (recvmm_buffers[i]->getDataSize() < sizeof(PacketHeader) + sizeof(PacketHeaderOption)) {
				LOG_ERROR("Packet too small {} < {}, DROPPING!!!", recvmm_buffers[i]->getDataSize(),
					 sizeof(PacketHeader) + sizeof(PacketHeaderOption));
				continue;
			}

			// Overlay packet header and do basic header checks
			pkthdr = (PacketHeader *)recvmm_buffers[i]->getData();
			// Check version is supported
			if (pkthdr->ver > SETH_PACKET_HEADER_VERSION_V1) {
				LOG_ERROR("Packet not supported, version {} vs. our version {}, DROPPING!", static_cast<uint8_t>(pkthdr->ver),
						  SETH_PACKET_HEADER_VERSION_V1);
				continue;
			}
			if (pkthdr->reserved != 0) {
				LOG_ERROR("Packet header should not have any reserved its set, it is {}, DROPPING!",
						  static_cast<uint8_t>(pkthdr->reserved));
				continue;
			}
			// First thing we do is validate the header
			int valid_format_mask =
				static_cast<uint8_t>(PacketHeaderFormat::ENCAPSULATED) | static_cast<uint8_t>(PacketHeaderFormat::COMPRESSED);
			if (static_cast<uint8_t>(pkthdr->format) & ~valid_format_mask) {
				LOG_ERROR("Packet in invalid format {}, DROPPING!", static_cast<uint8_t>(pkthdr->format));
				continue;
			}
			// Next check the channel is set to 0
			if (pkthdr->channel) {
				LOG_ERROR("Packet specifies invalid channel {}, DROPPING!", pkthdr->channel);
				continue;
			}

			// Add buffer node to the recycle list
			decoder.decode(std::move(recvmm_buffers[i]));

			// Replenish the buffer
			recvmm_buffers[i] = buffer_pool->pop();
			msgs[i].msg_hdr.msg_iov->iov_base = recvmm_buffers[i]->getData();
		}

		size_t i{0};
		auto buffers = decoder_pool->pop(0);
		for (auto &buffer : buffers) {
			// Write data to TAP interface
			ssize_t bytes_written = write(tdata->tap_device.fd, buffer->getData(), buffer->getDataSize());
			if (bytes_written == -1) {
				LOG_ERROR("Error writing TAP device: {}", strerror(errno));
				exit(EXIT_FAILURE);
			}

			i++;
			LOG_DEBUG_INTERNAL("Wrote {} bytes to TAP [{}/{}]", bytes_written, i, buffers.size());
		}
		// Push buffers into available pool
		buffer_pool->push(buffers);
	}

	// Add buffers back to available list for de-allocation
	for (uint32_t i = 0; i < SETH_MAX_RECVMM_MESSAGES; ++i) {
		buffer_pool->push(std::move(recvmm_buffers[i]));
	}
	// Free the rest of the IOV stuff
	free(msgs);
	free(iovecs);
}

