/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "codec.hpp"
#include "libaccl/buffer_pool.hpp"
#include "libaccl/logger.hpp"
#include "libaccl/stream_compressor_lz4.hpp"
#include "libaccl/stream_compressor_zstd.hpp"
#include "packet_buffer.hpp"
#include <deque>
#include <memory>

/*
 * Packet decoder
 */

class PacketDecoder {

	public:
		PacketDecoder(uint16_t l2mtu, std::shared_ptr<accl::BufferPool<PacketBuffer>> tx_buffer_pool,
					  std::shared_ptr<accl::BufferPool<PacketBuffer>> available_buffer_pool);
		~PacketDecoder();

		void decode(std::unique_ptr<PacketBuffer> packetBuffer);

		inline void setLastSequence(uint32_t seq);
		inline uint32_t getLastSequence() const;

	private:
		// Interface MTU
		uint16_t l2mtu;

		// Sequence counter
		bool first_packet;
		uint32_t last_sequence;
		uint8_t last_part;
		PacketHeaderOptionFormatType last_format;
		uint16_t last_orig_packet_size;
		// Active TX buffer that is not yet full
		std::unique_ptr<PacketBuffer> tx_buffer;
		// Buffers in flight, currently being utilized
		std::deque<std::unique_ptr<PacketBuffer>> inflight_buffers;

		// Compressor
		accl::StreamCompressorLZ4 *compressorLZ4;
		accl::StreamCompressorZSTD *compressorZSTD;
		std::unique_ptr<PacketBuffer> dcomp_buffer;

		// Buffer pool to push buffers to
		std::shared_ptr<accl::BufferPool<PacketBuffer>> tx_buffer_pool;
		// Buffer pool to get buffers from
		std::shared_ptr<accl::BufferPool<PacketBuffer>> available_buffer_pool;

		void _clearState();

		void _getTxBuffer();

		void _flushInflight();
		void _pushInflight(std::unique_ptr<PacketBuffer> &packetBuffer);
		void _clearStateAndFlushInflight(std::unique_ptr<PacketBuffer> &packetBuffer);
};

/**
 * @brief Set last packet sequence.
 *
 * @param seq Sequence to set.
 */
inline void PacketDecoder::setLastSequence(uint32_t seq) {
	LOG_DEBUG_INTERNAL("Setting decoder last sequence to ", seq);
	last_sequence = seq;
}

/**
 * @brief Get last packet sequence.
 *
 * @return uint32_t Last packet sequence.
 */
inline uint32_t PacketDecoder::getLastSequence() const { return last_sequence; }