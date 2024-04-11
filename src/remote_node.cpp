/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "remote_node.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "libaccl/logger.hpp"
#include "packet_buffer.hpp"
#include "util.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <memory>
#include <sys/socket.h>
#include <sys/types.h>

RemoteNode::RemoteNode(int udp_socket, const std::shared_ptr<sockaddr_storage> node_addr, int tx_size, int l2mtu, int buffer_size,
					   PacketHeaderOptionFormatType packet_format, std::shared_ptr<accl::BufferPool<PacketBuffer>> tap_write_pool,
					   std::shared_ptr<accl::BufferPool<PacketBuffer>> available_rx_buffer_pool,
					   std::shared_ptr<accl::BufferPool<PacketBuffer>> available_tx_buffer_pool, bool *stop_flag) {

	// Set UDP socket
	this->udp_socket = udp_socket;
	// Set remote node destination address
	this->node_addr = to_sockaddr_storage_ipv6(node_addr);
	// Set tx_size
	this->tx_size = tx_size;

	// Set L2MTU size
	this->l2mtu = l2mtu;

	/*
	 * Set L4MTU size
	 */
	this->l4mtu = this->tx_size;
	// Reduce by IP header size
	if (is_ipv4(node_addr))
		this->l4mtu -= 20; // IPv4 header
	else
		this->l4mtu -= 40; // IPv6 header
	this->l4mtu -= 8;	   // UDP frame

	// Set buffer size
	this->buffer_size = buffer_size;
	// Set packet format
	this->packet_format = packet_format;

	// Set this nodes key
	this->node_key = get_key_from_sockaddr(node_addr.get());

	// Decoder and encoder poools
	this->decoder_pool = std::make_shared<accl::BufferPool<PacketBuffer>>(this->buffer_size);
	this->encoder_pool = std::make_shared<accl::BufferPool<PacketBuffer>>(this->buffer_size);

	// Socket and TAP pools
	this->socket_write_pool = std::make_shared<accl::BufferPool<PacketBuffer>>(this->buffer_size);
	this->tap_write_pool = tap_write_pool;

	// Set available buffer pool
	this->available_rx_buffer_pool = available_rx_buffer_pool;
	this->available_tx_buffer_pool = available_tx_buffer_pool;

	// Blank the socket write thread pointer
	this->tunnel_socket_write_thread = nullptr;

	// Link stop flag
	this->stop_flag = stop_flag;
}

/**
 * @brief Start the remote node.
 */
void RemoteNode::start() {
	LOG_DEBUG_INTERNAL("Starting remote node ", get_ipstr(this->node_addr.get()));

	// Create the socket write thread
	this->tunnel_decoder_thread = std::make_unique<std::thread>(&RemoteNode::_tunnel_decoder_handler, this);
	this->tunnel_encoder_thread = std::make_unique<std::thread>(&RemoteNode::_tunnel_encoder_handler, this);
	this->tunnel_socket_write_thread = std::make_unique<std::thread>(&RemoteNode::_socket_write_handler, this);

	// Setup priority for a thread
	int policy = SCHED_RR;
	struct sched_param param;
	param.sched_priority = sched_get_priority_max(SCHED_RR);
	if (pthread_setschedparam(tunnel_decoder_thread->native_handle(), policy, &param)) {
		LOG_NOTICE("Could not set decoder thread priority: ", std::strerror(errno));
	}
	if (pthread_setschedparam(tunnel_encoder_thread->native_handle(), policy, &param)) {
		LOG_NOTICE("Could not set encoder thread priority: ", std::strerror(errno));
	}
	if (pthread_setschedparam(this->tunnel_socket_write_thread->native_handle(), policy, &param)) {
		LOG_NOTICE("Could not set socket write thread priority: ", std::strerror(errno));
	}
}

/**
 * @brief Stop the remote node.
 */
void RemoteNode::wait() {
	// Join the threads
	this->tunnel_decoder_thread->join();
	this->tunnel_encoder_thread->join();
	this->tunnel_socket_write_thread->join();
}

/**
 * @brief Thread responsible for decoding packets.
 *
 * @param arg Thread data.
 */
void RemoteNode::_tunnel_decoder_handler() {
	LOG_DEBUG_INTERNAL("DECODER: Starting decoder thread");

	PacketDecoder decoder(this->l2mtu, this->tap_write_pool, this->available_tx_buffer_pool);

	// Loop pulling buffers off the socket write pool
	std::deque<std::unique_ptr<PacketBuffer>> buffers;
	while (true) {
		// Check for program stop
		if (*this->stop_flag) {
			break;
		}

		// Wait for buffers
		this->decoder_pool->wait(buffers);

		// Loop with buffers
		for (auto &buffer : buffers) {
			decoder.decode(std::move(buffer));
		}
		buffers.clear();
	}

	LOG_DEBUG_INTERNAL("DECODER: Exiting decoder thread for node ", get_ipstr(this->node_addr.get()));
}

/**
 * @brief Thread responsible for encoding packets.
 *
 * @param arg Thread data.
 */
void RemoteNode::_tunnel_encoder_handler() {
	LOG_DEBUG_INTERNAL("ENCODER: Starting encoder thread");

	// Packet encoder
	PacketEncoder encoder(this->l2mtu, this->l4mtu, this->socket_write_pool, this->available_rx_buffer_pool);

	// Set packet format
	encoder.setPacketFormat(this->packet_format);

	// Statistics thread
	// std::thread stats_thread(tunnel_encoder_stats_handler, &encoder, tdata);

	// Loop pulling buffers off the encoder pool
	std::chrono::milliseconds timeout = std::chrono::milliseconds(1);
	std::deque<std::unique_ptr<PacketBuffer>> buffers;
	while (true) {
		// Check for program stop
		if (*this->stop_flag) {
			break;
		}

		// Check if we timed out
		if (!this->encoder_pool->wait_for(timeout, buffers)) {
			// If we did, flush the encoder and zero the wait so we wait indefinitely
			encoder.flush();
			timeout = std::chrono::milliseconds::zero();
			continue;
		}

		LOG_DEBUG_INTERNAL("NKDEBUG: Got ", buffers.size(), " buffers from encoder pool");

		// Loop with the buffers we got
		for (auto &buffer : buffers) {
			encoder.encode(std::move(buffer));
		}
		// Clear buffers
		buffers.clear();

		// Set timeout to 1ms
		timeout = std::chrono::milliseconds(1);
	}

	// Wait for stats thread to finish
	// stats_thread.join();

	LOG_DEBUG_INTERNAL("ENCODER: Exiting encoder thread for node ", get_ipstr(this->node_addr.get()));
}

/**
 * @brief Thread responsible for writing encoded packets to the socket.
 *
 * @param arg Thread data.
 */
void RemoteNode::_socket_write_handler() {
	LOG_DEBUG_INTERNAL("SOCKET WRITE: Starting socket write thread");

	// Constant used below
	socklen_t addrlen = sizeof(struct sockaddr_in6);
	// sendto() flags
	int flags = 0;

	// Loop pulling buffers off the socket write pool
	std::deque<std::unique_ptr<PacketBuffer>> buffers;
	while (true) {
		// Check for program stop
		if (*this->stop_flag) {
			break;
		}

		// Wait for buffers
		this->socket_write_pool->wait(buffers);

		// Loop with the buffers we got
#ifdef DEBUG
		size_t i{0};
#endif
		for (auto &buffer : buffers) {

			// FIXME
			const sockaddr_storage *addr = this->node_addr.get();

			std::string ipstr = get_ipstr(addr);
			LOG_DEBUG_INTERNAL("SOCKET WRITE: Writing ", buffer->getDataSize(), " bytes to SOCKET => ", ipstr);

			// Write data out
			ssize_t bytes_written =
				sendto(this->udp_socket, buffer->getData(), buffer->getDataSize(), flags, (struct sockaddr *)addr, addrlen);
			if (bytes_written == -1) {
				LOG_ERROR("Got an error in sendto(): ", strerror(errno));
			}
#ifdef DEBUG
			i++;
			LOG_DEBUG_INTERNAL("Wrote ", bytes_written, " bytes to tunnel [buffer: ", i, "/", buffers.size(), "]");
#endif
		}

		// Push buffers into available pool
		this->available_rx_buffer_pool->push(buffers);
	}

	LOG_DEBUG_INTERNAL("SOCKET WRITE: Exiting socket write thread for node ", get_ipstr(this->node_addr.get()));
}
