/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "encoder.hpp"
#include "codec.hpp"
#include "libaccl/buffer_pool.hpp"
#include "libaccl/logger.hpp"
#include "libaccl/stream_compressor_lz4.hpp"
#include "libaccl/stream_compressor_zstd.hpp"
#include "packet_buffer.hpp"
#include <cstdint>
#include <iomanip>
#include <memory>

/*
 * Packet encoder
 */

/**
 * @brief Internal method to flush the current buffer.
 *
 */
void PacketEncoder::_flush() {
	// Check if we have data in the tx_buffer, other than the header we add automatically
	if (this->tx_buffer->getDataSize() == sizeof(PacketHeader))
		return;

	// Check sequence is not about to wrap
	if (this->sequence == UINT32_MAX) {
		this->sequence = 1;
	}

	// The last thing we're doing is writing the packet header before dumping it into the this->tx_buffer_pool
	PacketHeader *packet_header = reinterpret_cast<PacketHeader *>(this->tx_buffer->getData());
	packet_header->ver = SETH_PACKET_HEADER_VERSION_V1;
	packet_header->opt_len = opt_len;
	packet_header->reserved = 0;
	packet_header->critical = 0;
	packet_header->oam = 0;
	packet_header->format = PacketHeaderFormat::ENCAPSULATED;
	packet_header->channel = 0;
	packet_header->sequence = accl::cpu_to_be_32(this->sequence++);

	// Dump the header into the tx buffer
	LOG_DEBUG_INTERNAL("{seq=", this->sequence - 1, "}:  - FLUSH: ADDING HEADER: opts=", this->opt_len);
	LOG_DEBUG_INTERNAL("{seq=", this->sequence - 1, "}:  - FLUSH: DEST BUFFER SIZE: ", this->tx_buffer->getDataSize());

	// Flush buffer to the tx pool
	this->tx_buffer_pool->push(std::move(this->tx_buffer));

	// Grab a new buffer
	this->_getTxBuffer();
}

/**
 * @brief Get another buffer and prepare it for encoded data.
 *
 */
void PacketEncoder::_getTxBuffer() {
	// Grab a buffer to use for the resulting packet
	this->tx_buffer = this->available_buffer_pool->pop_wait();
	tx_buffer->clear();
	// Advanced past header...
	tx_buffer->setDataSize(sizeof(PacketHeader));
	// Clear opt_len
	this->opt_len = 0;
	// Reset packet count to 0
	this->packet_count = 0;
}

/**
 * @brief Get maximum payload size depending on the size of the buffer that should be encoded.
 *
 * @param size Size of the packet to be encoded or SETH_PACKET_MAX_SIZE if unknown.
 * @return uint16_t Maximum payload size.
 */
uint16_t PacketEncoder::_getMaxPayloadSize(uint16_t size) const {
	int32_t max_payload_size = this->l4mtu;

	// If there is no data in the buffer, we're going to need a packet header
	if (!this->tx_buffer->getDataSize()) {
		max_payload_size -= sizeof(PacketHeader);
	}
	// Each packet needs a packet header
	max_payload_size -= sizeof(PacketHeaderOption);

	// Next we add the current buffer size, as this is overhead we've already incurred
	max_payload_size -= this->tx_buffer->getDataSize();

	// Intermdiate check if we've got no space
	if (max_payload_size <= 0) {
		return 0;
	}

	// Last check to see if we're below zero
	if (max_payload_size < 0) {
		max_payload_size = 0;
	}

	return static_cast<uint16_t>(max_payload_size);
}

/**
 * @brief Flush inflight buffers to the buffer pool.
 *
 */
void PacketEncoder::_flushInflight() {

	// Free remaining buffers in flight
	if (this->inflight_buffers.empty()) {
		LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:  - INFLIGHT: No buffers in flight to flush");
		return;
	}

	LOG_DEBUG_INTERNAL("{seq=", this->sequence,
					   "}:  - INFLIGHT: Flusing inflight buffers: avail pool=", this->available_buffer_pool->getBufferCount(),
					   ", inflight count=", this->inflight_buffers.size());

	// Flush inflight buffers to the buffer pool
	this->available_buffer_pool->push(this->inflight_buffers);

	// Reset compressor if we have one
	if (this->compressor) {
		LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:  - INFLIGHT: Resetting compressor");
		this->compressor->resetCompressionStream();
	}

	LOG_DEBUG_INTERNAL("{seq=", this->sequence,
					   "}:  - INFLIGHT: Flusing inflight buffers after: avail pool=", this->available_buffer_pool->getBufferCount(),
					   ", inflight count=", this->inflight_buffers.size());
}

/**
 * @brief Add a buffer to the inflight buffer pool.
 *
 * @param packetBuffer Buffer to add to the inflight buffer pool.
 */
void PacketEncoder::_pushInflight(std::unique_ptr<PacketBuffer> &packetBuffer) {
	// Push buffer into inflight list
	this->inflight_buffers.push_back(std::move(packetBuffer));
	LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:  - INFLIGHT: Packet added");
}

/**
 * @brief Construct a new Packet Encoder:: Packet Encoder object
 *
 * @param l2mtu Layer 2 MTU.
 * @param l4mtu Layer 4 MTU.
 * @param tx_buffer_pool TX buffer pool.
 * @param available_buffer_pool Available buffer pool.
 */
PacketEncoder::PacketEncoder(uint16_t l2mtu, uint16_t l4mtu, std::shared_ptr<accl::BufferPool<PacketBuffer>> tx_buffer_pool,
							 std::shared_ptr<accl::BufferPool<PacketBuffer>> available_buffer_pool)
	: statCompressionRatio(1000) {

	// As the constructor parameters have the same names as our data members, lets just use this-> for everything during init
	this->l2mtu = l2mtu;
	this->l4mtu = l4mtu;
	this->sequence = 1;

	// Initialize our compressor
	this->packet_format = PacketHeaderOptionFormatType::NONE;
	this->compressor = nullptr;

	// Grab a buffer to use for the compressor
	this->comp_buffer = available_buffer_pool->pop_wait();

	// Setup buffer pools
	this->tx_buffer_pool = tx_buffer_pool;
	this->available_buffer_pool = available_buffer_pool;

	// Initialize the tx buffer for first use
	this->_getTxBuffer();
}

/**
 * @brief Destroy the Packet Encoder:: Packet Encoder object
 *
 */
PacketEncoder::~PacketEncoder() {
	if (this->compressor) {
		delete this->compressor;
	};
};

/**
 * @brief Encode a packet buffer.
 *
 * @param packetBuffer Packet buffer to encode.
 */
void PacketEncoder::encode(std::unique_ptr<PacketBuffer> rawPacketBuffer) {
	// Save the original size
	uint16_t original_size = rawPacketBuffer->getDataSize();

	LOG_DEBUG_INTERNAL("====================");
	LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}: INCOMING PACKET: size=", original_size, " [l2mtu: ", this->l2mtu,
					   ", l4mtu: ", this->l4mtu, "], buffer_size=", tx_buffer->getDataSize(), ", packet_count=", this->packet_count,
					   ", opt_len=", this->opt_len);

	// Make sure we cannot receive a packet that is greater than our MTU size
	if (original_size > this->l2mtu) {
		LOG_ERROR("Packet size ", original_size, " exceeds L2MTU size ", this->l2mtu, "?");
		// Free buffer to available pool
		this->available_buffer_pool->push(std::move(rawPacketBuffer));
		return;
	}

	// Create a packet buffer pointer, this is what we use to point to the data queued to encode
	std::unique_ptr<PacketBuffer> packetBuffer;
	uint8_t packet_header_option_format = static_cast<uint8_t>(this->packet_format);

	// Check if we need to compress the packet
	if (this->packet_format != PacketHeaderOptionFormatType::NONE) {
		// Compress th raw packet buffer we got into the compressed packet buffer
		int compressed_size = this->compressor->compress(rawPacketBuffer->getData(), original_size, this->comp_buffer->getData(),
														 this->comp_buffer->getBufferSize());
		// Check if we were indeed compressed
		// NK: We don't care if our size exceeds the original size as the header savings would be worth it if there is a second
		// packet coming
		if (compressed_size) {
			// Mark the packet as compressed
			packet_header_option_format = static_cast<uint8_t>(this->packet_format);
			LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:  - COMPRESSED: size=", compressed_size,
							   ", format=", static_cast<unsigned int>(packet_header_option_format));
			this->comp_buffer->setDataSize(compressed_size);
			// Set the packet buffer we're working with
			packetBuffer = std::move(this->comp_buffer);
			this->comp_buffer = nullptr;
			// Add buffer to inflight list, we need to keep it around for the duration of the stream compression run as some
			// algorithms require all buffers to be available.
			_pushInflight(rawPacketBuffer);
		} else {
			LOG_ERROR("{seq=", this->sequence, "}: Failed to compress packet with error ", compressed_size, ": ",
					  this->compressor->strerror(compressed_size));
			// Set packet header to uncompressed
			packet_header_option_format = static_cast<uint8_t>(PacketHeaderOptionFormatType::NONE);
			// Set the packet buffer we're working with
			packetBuffer = std::move(rawPacketBuffer);
		}

		// Update compression ratio statistic percentage
		this->statCompressionRatio.add(static_cast<float>(compressed_size) / static_cast<float>(original_size) * 100.0f);
		LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:  - COMPRESSION RATIO: ", std::fixed, std::setprecision(2),
						   static_cast<float>(compressed_size) / static_cast<float>(original_size) * 100.0f);

	} else {
		packetBuffer = std::move(rawPacketBuffer);
	}

	// Handle packets that are larger than the MSS
	if (packetBuffer->getDataSize() > this->_getMaxPayloadSize(packetBuffer->getDataSize())) {
		// Start off at packet part 1 and position 0...
		uint8_t part = 1;
		uint16_t packet_pos = 0; // Position in the packet we're stuffing into the encap packet

		LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}: - ENCODE PARTIAL PACKET - ");

		// Loop while we still have bits of the packet to stuff into the encap packets
		while (packet_pos < packetBuffer->getDataSize()) {
			// Amount of data left in the packet still to encode
			uint16_t packet_left = packetBuffer->getDataSize() - packet_pos;
			// Maximum payload size for the current encap packet
			uint16_t max_payload_size = this->_getMaxPayloadSize(packet_left);
			// Work out how much of this packet we're going to stuff into the encap packet
			uint16_t part_size = (packet_left > max_payload_size) ? max_payload_size : packet_left;

			LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:  - PARTIAL packet loop: max_payload_size=", max_payload_size,
							   ", part=", static_cast<unsigned int>(part), ", packet_pos=", packet_pos, ", part_size=", part_size);

			// Grab current buffer size so we can do some magic memory meddling below
			size_t cur_buffer_size = tx_buffer->getDataSize();

			PacketHeaderOption *packet_header_option =
				reinterpret_cast<PacketHeaderOption *>(tx_buffer->getData() + cur_buffer_size);
			packet_header_option->type = PacketHeaderOptionType::PARTIAL_PACKET;
			packet_header_option->packet_size = accl::cpu_to_be_16(original_size);
			packet_header_option->format = static_cast<PacketHeaderOptionFormatType>(packet_header_option_format);
			packet_header_option->payload_length = accl::cpu_to_be_16(part_size);
			packet_header_option->part = part;
			packet_header_option->reserved = 0;

			// Add packet header option
			cur_buffer_size += sizeof(PacketHeaderOption);
			// Only bump option len if this is the first packet at the beginning of the encap packet
			if (!this->packet_count) {
				this->opt_len++;
			}
			// Bump packet count
			this->packet_count++;

			// Once we're done messing with the buffer, set its size
			this->tx_buffer->setDataSize(cur_buffer_size);

			// Dump more of the packet into our tx buffer
			this->tx_buffer->append(packetBuffer->getData() + packet_pos, part_size);

			LOG_DEBUG_INTERNAL("{seq=", this->sequence,
							   "}:    - After partial add: tx_buffer_size=", this->tx_buffer->getDataSize());

			// Check if we have finished the packet, if we have, mark this one complete
			if (!(packet_left - part_size)) {
				LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:    - Buffer not full, setting complete");
				// Set packet header to complete
				packet_header_option->type = PacketHeaderOptionType(static_cast<uint8_t>(packet_header_option->type) |
																	static_cast<uint8_t>(PacketHeaderOptionType::COMPLETE_PACKET));
			}

			// If the buffer is full, flush it
			if (this->tx_buffer->getDataSize() == this->l4mtu) {
				LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:    - Buffer full, flushing");
				this->_flush();
			}

			// Adjust packet position
			packet_pos += part_size;
			part++;
		}

		// Push buffer into inflight list
		// _pushInflight(packetBuffer);
		this->_flushInflight();

		LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:  - PARTIAL END: buffer size is ", this->tx_buffer->getDataSize(),
						   " (packets: ", this->packet_count, ")");

	} else {
		// Grab current buffer size so we can do some magic memory meddling below
		size_t cur_buffer_size = this->tx_buffer->getDataSize();

		LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}: - ENCODE COMPLETE PACKET - ");

		PacketHeaderOption *packet_header_option =
			reinterpret_cast<PacketHeaderOption *>(this->tx_buffer->getData() + cur_buffer_size);
		packet_header_option->type = PacketHeaderOptionType::COMPLETE_PACKET;
		packet_header_option->packet_size = accl::cpu_to_be_16(original_size);
		packet_header_option->format = static_cast<PacketHeaderOptionFormatType>(packet_header_option_format);
		packet_header_option->payload_length = accl::cpu_to_be_16(packetBuffer->getDataSize());
		packet_header_option->part = 0;
		packet_header_option->reserved = 0;

		LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:  - OPTION HEADER: packet_bufer_size=", packetBuffer->getDataSize(),
						   ", max_payload_size=", this->_getMaxPayloadSize(packetBuffer->getDataSize()),
						   ", header_option_size=", sizeof(PacketHeaderOption), ", tx_buffer_size=", cur_buffer_size);

		// Add packet header option
		cur_buffer_size += sizeof(PacketHeaderOption);
		// Only bump option len if this is the first packet at the beginning of the encap packet
		if (!this->packet_count) {
			this->opt_len++;
		}
		// Bump packet count
		this->packet_count++;

		// Once we're done messing with the buffer, set its size
		this->tx_buffer->setDataSize(cur_buffer_size);

		this->tx_buffer->append(packetBuffer->getData(), packetBuffer->getDataSize());
		// // Set packet buffer to being inflight
		// _pushInflight(packetBuffer);

		LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:  - FINAL DEST BUFFER SIZE: ", this->tx_buffer->getDataSize(),
						   " (packets: ", this->packet_count, ")");
	}

	// If the buffer is full, flush it
	// NK: We need to make provision for a header
	LOG_DEBUG_INTERNAL("{seq=", this->sequence, "}:   - Flush check: ", this->_getMaxPayloadSize(SETH_PACKET_MAX_SIZE), " < ",
					   sizeof(PacketHeader) + (sizeof(PacketHeaderOption) * 10));
	if (this->_getMaxPayloadSize(SETH_PACKET_MAX_SIZE) < sizeof(PacketHeader) + (sizeof(PacketHeaderOption) * 10)) {
		// Packet is as full as it will get
		this->_flush();
		this->_flushInflight();
	}

	// Check if we need a compressed packet buffer
	if (this->packet_format != PacketHeaderOptionFormatType::NONE && this->comp_buffer == nullptr) {
		// If we do, we can re-use the packet buffer
		this->comp_buffer = std::move(packetBuffer);
	} else {
		this->available_buffer_pool->push(std::move(packetBuffer));
	}
}

/**
 * @brief Public method to flush the current buffer.
 *
 */
void PacketEncoder::flush() {
	// When we get a flush, it means we probably can't fill the buffer anymore, so we can just dump it and the inflight buffers too
	this->_flush();
	this->_flushInflight();
}

/**
 * @brief Set packet format.
 *
 * @param format Packet format to set.
 */
void PacketEncoder::setPacketFormat(PacketHeaderOptionFormatType format) {
	// Set packet format
	this->packet_format = format;

	// Initialize compressor
	if (this->packet_format == PacketHeaderOptionFormatType::NONE) {
		// No compression
		this->compressor = nullptr;
	} else if (this->packet_format == PacketHeaderOptionFormatType::COMPRESSED_LZ4) {
		// Initialize our compressor
		this->compressor = new accl::StreamCompressorLZ4();
	} else if (this->packet_format == PacketHeaderOptionFormatType::COMPRESSED_ZSTD) {
		// Initialize our compressor
		this->compressor = new accl::StreamCompressorZSTD();
	} else {
		LOG_ERROR("Unknown packet format ", static_cast<unsigned int>(format));
		throw std::runtime_error("Unknown packet format");
	}
}

/**
 * @brief Get compression ratio statistic.
 *
 * @return accl::Statistic<float>& Compression ratio statistic.
 */
void PacketEncoder::getCompressionRatioStat(accl::StatisticResult<float> &result) {
	this->statCompressionRatio.getStatisticResult(result);
}
