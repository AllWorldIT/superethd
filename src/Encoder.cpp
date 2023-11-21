/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "Encoder.hpp"
#include "Codec.hpp"
#include "Endian.hpp"
#include "PacketBuffer.hpp"
#include "libaccl/Logger.hpp"
#include "libaccl/StreamCompressorBlosc2.hpp"
#include "libaccl/StreamCompressorLZ4.hpp"
#include "libsethnetkit/Packet.hpp"
#include <cstdint>

/*
 * Packet encoder
 */

/**
 * @brief Internal method to flush the current buffer.
 *
 */
void PacketEncoder::_flush() {
	// Check if we have data in the tx_buffer, other than the header we add automatically
	if (tx_buffer->getDataSize() == sizeof(PacketHeader))
		return;

	// Check sequence is not about to wrap
	if (sequence == UINT32_MAX) {
		sequence = 1;
	}

	// The last thing we're doing is writing the packet header before dumping it into the tx_buffer_pool
	PacketHeader *packet_header = reinterpret_cast<PacketHeader *>(tx_buffer->getData());
	packet_header->ver = SETH_PACKET_HEADER_VERSION_V1;
	packet_header->opt_len = opt_len;
	packet_header->reserved = 0;
	packet_header->critical = 0;
	packet_header->oam = 0;
	packet_header->format = PacketHeaderFormat::ENCAPSULATED;
	packet_header->channel = 0;
	packet_header->sequence = seth_cpu_to_be_32(sequence++);

	// Dump the header into the tx buffer
	LOG_DEBUG_INTERNAL("{seq=", sequence - 1, "}:  - FLUSH: ADDING HEADER: opts=", opt_len);
	LOG_DEBUG_INTERNAL("{seq=", sequence - 1, "}:  - FLUSH: DEST BUFFER SIZE: ", tx_buffer->getDataSize());

	// Flush buffer to the tx pool
	tx_buffer_pool->push(std::move(tx_buffer));

	// Grab a new buffer
	_getTxBuffer();
}

/**
 * @brief Get another buffer and prepare it for encoded data.
 *
 */
void PacketEncoder::_getTxBuffer() {
	// Grab a buffer to use for the resulting packet
	tx_buffer = buffer_pool->pop_wait();
	tx_buffer->clear();
	// Advanced past header...
	tx_buffer->setDataSize(sizeof(PacketHeader));
	// Clear opt_len
	opt_len = 0;
	// Reset packet count to 0
	packet_count = 0;
}

/**
 * @brief Get maximum payload size depending on the size of the buffer that should be encoded.
 *
 * @param size Size of the packet to be encoded or SETH_PACKET_MAX_SIZE if unknown.
 * @return uint16_t Maximum payload size.
 */
uint16_t PacketEncoder::_getMaxPayloadSize(uint16_t size) const {
	int32_t max_payload_size = l4mtu;

	// If there is no data in the buffer, we're going to need a packet header
	if (!tx_buffer->getDataSize()) {
		max_payload_size -= sizeof(PacketHeader);
	}
	// Each packet needs a packet header
	max_payload_size -= sizeof(PacketHeaderOption);

	// Next we add the current buffer size, as this is overhead we've already incurred
	max_payload_size -= tx_buffer->getDataSize();

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
	LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - INFLIGHT: Flusing inflight buffers: pool=", buffer_pool->getBufferCount(),
					   ", count=", inflight_buffers.size());
	// Free remaining buffers in flight
	if (!inflight_buffers.empty()) {
		buffer_pool->push(inflight_buffers);
	}

	// Reset compressor if we have one
	if (compressor) {
		LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - INFLIGHT: Resetting compressor");
		compressor->resetCompressionStream();
	}
	
	LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - INFLIGHT: Flusing inflight buffers after: pool=", buffer_pool->getBufferCount(),
					   ", count=", inflight_buffers.size());
}

/**
 * @brief Add a buffer to the inflight buffer pool.
 *
 * @param packetBuffer Buffer to add to the inflight buffer pool.
 */
void PacketEncoder::_pushInflight(std::unique_ptr<PacketBuffer> &packetBuffer) {
	// Push buffer into inflight list
	inflight_buffers.push_back(std::move(packetBuffer));
	LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - INFLIGHT: Packet added");
}

/**
 * @brief Construct a new Packet Encoder:: Packet Encoder object
 *
 * @param l2mtu Layer 2 MTU.
 * @param l4mtu Layer 4 MTU.
 * @param available_buffer_pool Available buffer pool.
 * @param tx_buffer_pool TX buffer pool.
 */
PacketEncoder::PacketEncoder(uint16_t l2mtu, uint16_t l4mtu, accl::BufferPool<PacketBuffer> *available_buffer_pool,
							 accl::BufferPool<PacketBuffer> *tx_buffer_pool) {
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
	this->buffer_pool = available_buffer_pool;
	this->tx_buffer_pool = tx_buffer_pool;

	// Initialize the tx buffer for first use
	_getTxBuffer();
}

/**
 * @brief Destroy the Packet Encoder:: Packet Encoder object
 *
 */
PacketEncoder::~PacketEncoder() {
	if (compressor) {
		delete compressor;
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
	LOG_DEBUG_INTERNAL("{seq=", sequence, "}: INCOMING PACKET: size=", original_size, " [l2mtu: ", l2mtu, ", l4mtu: ", l4mtu,
					   "], buffer_size=", tx_buffer->getDataSize(), ", packet_count=", packet_count, ", opt_len=", opt_len);

	// Make sure we cannot receive a packet that is greater than our MTU size
	if (original_size > l2mtu) {
		LOG_ERROR("Packet size ", original_size, " exceeds L2MTU size ", l2mtu, "?");
		// Free buffer to available pool
		buffer_pool->push(std::move(rawPacketBuffer));
		return;
	}

	// Create a packet buffer pointer, this is what we use to point to the data queued to encode
	std::unique_ptr<PacketBuffer> packetBuffer;
	uint8_t packet_header_option_format = static_cast<uint8_t>(packet_format);

	// Check if we need to compress the packet
	if (packet_format != PacketHeaderOptionFormatType::NONE) {
		// Compress th raw packet buffer we got into the compressed packet buffer
		int compressed_size =
			compressor->compress(rawPacketBuffer->getData(), original_size, comp_buffer->getData(), comp_buffer->getBufferSize());
		// Check if we were indeed compressed
		// NK: We don't care if our size exceeds the original size as the header savings would be worth it if there is a second
		// packet coming
		if (compressed_size) {
			// Mark the packet as compressed
			packet_header_option_format = static_cast<uint8_t>(packet_format);
			LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - COMPRESSED: size=", compressed_size,
							   ", format=", static_cast<unsigned int>(packet_header_option_format));
			comp_buffer->setDataSize(compressed_size);
			// Set the packet buffer we're working with
			packetBuffer = std::move(comp_buffer);
			comp_buffer = nullptr;
			// Add buffer to inflight list, we need to keep it around for the duration of the stream compression run as some
			// algorithms require all buffers to be available.
			_pushInflight(rawPacketBuffer);
		} else {
			LOG_ERROR("{seq=", sequence, "}: Failed to compress packet");
			// Free buffer to available pool
			// buffer_pool->push(std::move(*compressed_packet_buffer));
			// Set the packet buffer we're working with
			packetBuffer = std::move(rawPacketBuffer);
		}
	} else {
		packetBuffer = std::move(rawPacketBuffer);
	}

	// Handle packets that are larger than the MSS
	if (packetBuffer->getDataSize() > _getMaxPayloadSize(packetBuffer->getDataSize())) {
		// Start off at packet part 1 and position 0...
		uint8_t part = 1;
		uint16_t packet_pos = 0; // Position in the packet we're stuffing into the encap packet

		LOG_DEBUG_INTERNAL("{seq=", sequence, "}: - ENCODE PARTIAL PACKET - ");

		// Loop while we still have bits of the packet to stuff into the encap packets
		while (packet_pos < packetBuffer->getDataSize()) {
			// Amount of data left in the packet still to encode
			uint16_t packet_left = packetBuffer->getDataSize() - packet_pos;
			// Maximum payload size for the current encap packet
			uint16_t max_payload_size = _getMaxPayloadSize(packet_left);
			// Work out how much of this packet we're going to stuff into the encap packet
			uint16_t part_size = (packet_left > max_payload_size) ? max_payload_size : packet_left;

			LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - PARTIAL packet loop: max_payload_size=", max_payload_size,
							   ", part=", static_cast<unsigned int>(part), ", packet_pos=", packet_pos, ", part_size=", part_size);

			// Grab current buffer size so we can do some magic memory meddling below
			size_t cur_buffer_size = tx_buffer->getDataSize();

			PacketHeaderOption *packet_header_option =
				reinterpret_cast<PacketHeaderOption *>(tx_buffer->getData() + cur_buffer_size);
			packet_header_option->type = PacketHeaderOptionType::PARTIAL_PACKET;
			packet_header_option->packet_size = seth_cpu_to_be_16(original_size);
			packet_header_option->format = static_cast<PacketHeaderOptionFormatType>(packet_header_option_format);
			packet_header_option->payload_length = seth_cpu_to_be_16(part_size);
			packet_header_option->part = part;
			packet_header_option->reserved = 0;

			// Add packet header option
			cur_buffer_size += sizeof(PacketHeaderOption);
			// Only bump option len if this is the first packet at the beginning of the encap packet
			if (!packet_count) {
				opt_len++;
			}
			// Bump packet count
			packet_count++;

			// Once we're done messing with the buffer, set its size
			tx_buffer->setDataSize(cur_buffer_size);

			// Dump more of the packet into our tx buffer
			tx_buffer->append(packetBuffer->getData() + packet_pos, part_size);

			LOG_DEBUG_INTERNAL("{seq=", sequence, "}:    - After partial add: tx_buffer_size=", tx_buffer->getDataSize());

			// If the buffer is full, flush it
			if (tx_buffer->getDataSize() == l4mtu) {
				LOG_DEBUG_INTERNAL("{seq=", sequence, "}:    - Buffer full, flushing");
				_flush();
			}

			// Adjust packet position
			packet_pos += part_size;
			part++;
		}

		// Push buffer into inflight list
		// _pushInflight(packetBuffer);
		_flushInflight();

		LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - PARTIAL END: buffer size is ", tx_buffer->getDataSize(),
						   " (packets: ", packet_count, ")");

	} else {
		// Grab current buffer size so we can do some magic memory meddling below
		size_t cur_buffer_size = tx_buffer->getDataSize();

		LOG_DEBUG_INTERNAL("{seq=", sequence, "}: - ENCODE COMPLETE PACKET - ");

		PacketHeaderOption *packet_header_option = reinterpret_cast<PacketHeaderOption *>(tx_buffer->getData() + cur_buffer_size);
		packet_header_option->type = PacketHeaderOptionType::COMPLETE_PACKET;
		packet_header_option->packet_size = seth_cpu_to_be_16(original_size);
		packet_header_option->format = static_cast<PacketHeaderOptionFormatType>(packet_header_option_format);
		packet_header_option->payload_length = seth_cpu_to_be_16(packetBuffer->getDataSize());
		packet_header_option->part = 0;
		packet_header_option->reserved = 0;

		LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - OPTION HEADER: packet_bufer_size=", packetBuffer->getDataSize(),
						   ", max_payload_size=", _getMaxPayloadSize(packetBuffer->getDataSize()),
						   ", header_option_size=", sizeof(PacketHeaderOption), ", tx_buffer_size=", cur_buffer_size);

		// Add packet header option
		cur_buffer_size += sizeof(PacketHeaderOption);
		// Only bump option len if this is the first packet at the beginning of the encap packet
		if (!packet_count) {
			opt_len++;
		}
		// Bump packet count
		packet_count++;

		// Once we're done messing with the buffer, set its size
		tx_buffer->setDataSize(cur_buffer_size);

		tx_buffer->append(packetBuffer->getData(), packetBuffer->getDataSize());
		// // Set packet buffer to being inflight
		// _pushInflight(packetBuffer);

		LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - FINAL DEST BUFFER SIZE: ", tx_buffer->getDataSize(),
						   " (packets: ", packet_count, ")");
	}

	// If the buffer is full, flush it
	// NK: We need to make provision for a header
	LOG_DEBUG_INTERNAL("{seq=", sequence, "}:   - Flush check: ", _getMaxPayloadSize(SETH_PACKET_MAX_SIZE), " < ",
					   sizeof(PacketHeader) + (sizeof(PacketHeaderOption) * 10));
	if (_getMaxPayloadSize(SETH_PACKET_MAX_SIZE) < sizeof(PacketHeader) + (sizeof(PacketHeaderOption) * 10)) {
		// Packet is as full as it will get
		_flush();
		_flushInflight();
	}

	// Check if we need a compressed packet buffer
	if (packet_format != PacketHeaderOptionFormatType::NONE && comp_buffer == nullptr) {
		// If we do, we can re-use the packet buffer
		comp_buffer = std::move(packetBuffer);
	} else {
		buffer_pool->push(std::move(packetBuffer));
	}
}

/**
 * @brief Public method to flush the current buffer.
 *
 */
void PacketEncoder::flush() {
	// When we get a flush, it means we probably can't fill the buffer anymore, so we can just dump it and the inflight buffers too
	_flush();
	_flushInflight();
}

/**
 * @brief Set packet format.
 *
 * @param format Packet format to set.
 */
void PacketEncoder::setPacketFormat(PacketHeaderOptionFormatType format) {
	// Set packet format
	packet_format = format;

	// Initialize compressor
	if (packet_format == PacketHeaderOptionFormatType::COMPRESSED_LZ4) {
		// Initialize our compressor
		compressor = new accl::StreamCompressorLZ4();
	} else if (packet_format == PacketHeaderOptionFormatType::COMPRESSED_BLOSC2) {
		// Initialize our compressor
		compressor = new accl::StreamCompressorBlosc2();
	} else {
		LOG_ERROR("Unknown packet format ", static_cast<unsigned int>(format));
		throw std::runtime_error("Unknown packet format");
	}
}
