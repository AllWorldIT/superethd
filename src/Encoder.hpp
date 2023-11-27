/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "Codec.hpp"
#include "PacketBuffer.hpp"
#include "libaccl/BufferPool.hpp"
#include "libaccl/Logger.hpp"
#include "libaccl/Statistic.hpp"
#include "libaccl/StreamCompressor.hpp"
#include <memory>

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
		// Active tx buffer that is not yet full
		std::unique_ptr<PacketBuffer> tx_buffer;
		// Buffers in flight, currently being utilized
		std::deque<std::unique_ptr<PacketBuffer>> inflight_buffers;

		// Compressor
		PacketHeaderOptionFormatType packet_format;
		accl::StreamCompressor *compressor;
		std::unique_ptr<PacketBuffer> comp_buffer;

		// Active tx buffer header options length
		uint16_t opt_len;
		// Number of packets currently encoded
		uint32_t packet_count;

		// Buffer pool to get buffers from
		accl::BufferPool<PacketBuffer> *buffer_pool;
		// TX buffer pool queued to send via socket
		accl::BufferPool<PacketBuffer> *tx_buffer_pool;

		// Statistics
		accl::Statistic<float> statCompressionRatio;

		void _flush();

		void _getTxBuffer();

		uint16_t _getMaxPayloadSize(uint16_t size) const;

		void _flushInflight();
		void _pushInflight(std::unique_ptr<PacketBuffer> &packetBuffer);

	public:
		PacketEncoder(uint16_t l2mtu, uint16_t l4mtu, accl::BufferPool<PacketBuffer> *available_buffer_pool,
					  accl::BufferPool<PacketBuffer> *tx_buffer_pool);
		~PacketEncoder();

		void encode(std::unique_ptr<PacketBuffer> packetBuffer);

		void flush();

		inline void setSequence(uint32_t seq);
		inline uint32_t getSequence() const;

		void setPacketFormat(PacketHeaderOptionFormatType format);
		inline PacketHeaderOptionFormatType getPacketFormat() const;

		void getCompressionRatioStat(accl::StatisticResult<float> &result);
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

/**
 * @brief Get current packet format.
 *
 * @return PacketHeaderOptionFormatType Packet format.
 */
inline PacketHeaderOptionFormatType PacketEncoder::getPacketFormat() const { return packet_format; }
