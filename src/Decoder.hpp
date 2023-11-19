/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "Codec.hpp"

/*
 * Packet decoder
 */

class PacketDecoder {
	private:
		// Interface MTU
		uint16_t l2mtu;

		// Sequence counter
		bool first_packet;
		uint32_t last_sequence;
		uint8_t last_part;
		uint16_t last_final_packet_size;
		// Active destination buffer that is not yet full
		std::unique_ptr<PacketBuffer> dest_buffer;
		// Buffers in flight, currently being utilized
		std::deque<std::unique_ptr<PacketBuffer>> inflight_buffers;
		// Buffer pool to get buffers from
		accl::BufferPool<PacketBuffer> *buffer_pool;
		// Buffer pool to push buffers to
		accl::BufferPool<PacketBuffer> *dest_buffer_pool;

		void _clearState();

		void _getDestBuffer();

		void _flushInflight();
		void _pushInflight(std::unique_ptr<PacketBuffer> &packetBuffer);
		void _clearStateAndFlushInflight(std::unique_ptr<PacketBuffer> &packetBuffer);

	public:
		PacketDecoder(uint16_t l2mtu, accl::BufferPool<PacketBuffer> *available_buffer_pool,
					  accl::BufferPool<PacketBuffer> *destination_buffer_pool);
		~PacketDecoder();

		void decode(std::unique_ptr<PacketBuffer> packetBuffer);

		inline void setLastSequence(uint32_t seq);
		inline uint32_t getLastSequence() const;
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