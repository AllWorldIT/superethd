/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "Codec.hpp"
#include "libaccl/Logger.hpp"

/*
 * Packet encoder
 */

class PacketEncoder {
	private:
		// Interface L2MTU
		uint16_t l2mtu;
		// Maximum layer 4 segment size, this is the maximum size our layer 4 transport can handle
		uint16_t l4mtu;

		// Sequence counter
		uint32_t sequence;
		// Active destination buffer that is not yet full
		std::unique_ptr<PacketBuffer> dest_buffer;
		// Buffers in flight, currently being utilized
		std::deque<std::unique_ptr<PacketBuffer>> inflight_buffers;
		// Active destination buffer header options length
		uint16_t opt_len;
		// Number of packets currently encoded
		uint32_t packet_count;

		// Buffer pool to get buffers from
		accl::BufferPool<PacketBuffer> *buffer_pool;
		// Buffer pool to push buffers to
		accl::BufferPool<PacketBuffer> *dest_buffer_pool;

		void _flush();

		void _getDestBuffer();

		uint16_t _getMaxPayloadSize(uint16_t size) const;

		void _flushInflight();
		void _pushInflight(std::unique_ptr<PacketBuffer> &packetBuffer);

	public:
		PacketEncoder(uint16_t l2mtu, uint16_t l4mtu, accl::BufferPool<PacketBuffer> *available_buffer_pool,
					  accl::BufferPool<PacketBuffer> *destination_buffer_pool);
		~PacketEncoder();

		void encode(std::unique_ptr<PacketBuffer> packetBuffer);

		void flush();

		inline void setSequence(uint32_t seq);
		inline uint32_t getSequence() const;
};

/**
 * @brief Set packet sequence.
 *
 * @param seq Packet sequence to set.
 */
inline void PacketEncoder::setSequence(uint32_t seq) {
	LOG_DEBUG_INTERNAL("Setting encoder sequence to ", seq);
	sequence = seq;
}

/**
 * @brief Get current packet sequence.
 *
 * @return uint32_t Packet sequence.
 */
inline uint32_t PacketEncoder::getSequence() const { return sequence; }