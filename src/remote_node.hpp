/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "codec.hpp"
#include <libaccl/buffer_pool.hpp>
#include <memory>
#include <netinet/in.h>
#include <packet_buffer.hpp>

class RemoteNode {
	public:
		RemoteNode(int udp_socket, const std::shared_ptr<sockaddr_storage> node_addr, int tx_size, int l2mtu, int buffer_size,
				   PacketHeaderOptionFormatType packet_format, std::shared_ptr<accl::BufferPool<PacketBuffer>> tap_write_pool,
				   std::shared_ptr<accl::BufferPool<PacketBuffer>> available_rx_buffer_pool,
				   std::shared_ptr<accl::BufferPool<PacketBuffer>> available_tx_buffer_pool, bool *stop_flag);

		void start();
		void wait();

		inline const std::array<uint8_t, 16> &getNodeKey() const;
		inline const std::shared_ptr<sockaddr_storage> getNodeAddr() const;

		inline std::shared_ptr<accl::BufferPool<PacketBuffer>> getDecoderPool();
		inline std::shared_ptr<accl::BufferPool<PacketBuffer>> getEncoderPool();
		inline std::shared_ptr<accl::BufferPool<PacketBuffer>> getSocketWritePool();

	private:
		int udp_socket;
		std::shared_ptr<sockaddr_storage> node_addr;
		uint16_t tx_size;
		uint16_t l2mtu;
		uint16_t l4mtu;
		int buffer_size;
		PacketHeaderOptionFormatType packet_format;

		// Node key used to index this node
		std::array<uint8_t, 16> node_key;

		// Buffer pool for decoder and encoder
		std::shared_ptr<accl::BufferPool<PacketBuffer>> decoder_pool;
		std::shared_ptr<accl::BufferPool<PacketBuffer>> encoder_pool;
		// Buffer pool for socket write
		std::shared_ptr<accl::BufferPool<PacketBuffer>> socket_write_pool;
		std::shared_ptr<accl::BufferPool<PacketBuffer>> tap_write_pool;
		// Available buffer pools
		std::shared_ptr<accl::BufferPool<PacketBuffer>> available_rx_buffer_pool;
		std::shared_ptr<accl::BufferPool<PacketBuffer>> available_tx_buffer_pool;

		// Buffer pool write thread
		std::unique_ptr<std::thread> tunnel_decoder_thread;
		std::unique_ptr<std::thread> tunnel_encoder_thread;
		std::unique_ptr<std::thread> tunnel_socket_write_thread;

		// Stop flag
		bool *stop_flag;

		void _tunnel_decoder_handler();
		void _tunnel_encoder_handler();
		void _socket_write_handler();
};

/**
 * @brief Get the Node Key object
 *
 * @return const std::array<uint8_t, 16>&
 */
inline const std::array<uint8_t, 16> &RemoteNode::getNodeKey() const { return this->node_key; }

/**
 * @brief Get the Node Addr object
 *
 * @return const std::shared_ptr<sockaddr_storage>
 */
inline const std::shared_ptr<sockaddr_storage> RemoteNode::getNodeAddr() const { return this->node_addr; }

/**
 * @brief Return this nodes decoder pool
 *
 * @return std::shared_ptr<accl::BufferPool<PacketBuffer>>
 */
inline std::shared_ptr<accl::BufferPool<PacketBuffer>> RemoteNode::getDecoderPool() { return this->decoder_pool; }

/**
 * @brief Return this nodes encoder pool
 *
 * @return std::shared_ptr<accl::BufferPool<PacketBuffer>>
 */
inline std::shared_ptr<accl::BufferPool<PacketBuffer>> RemoteNode::getEncoderPool() { return this->encoder_pool; }

/**
 * @brief Return this nodes socket write pool
 *
 * @return std::shared_ptr<accl::BufferPool<PacketBuffer>>
 */
inline std::shared_ptr<accl::BufferPool<PacketBuffer>> RemoteNode::getSocketWritePool() { return this->socket_write_pool; }
