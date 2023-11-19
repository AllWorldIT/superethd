/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "Encoder.hpp"
#include <cstdint>

/*
 * Packet encoder
 */

/**
 * @brief Internal method to flush the current buffer.
 *
 */
void PacketEncoder::_flush() {
	// Check if we have data in the dest_buffer, other than the header we add automatically
	if (dest_buffer->getDataSize() == sizeof(PacketHeader))
		return;

	// Check sequence is not about to wrap
	if (sequence == UINT32_MAX) {
		sequence = 1;
	}

	// The last thing we're doing is writing the packet header before dumping it into the dest_buffer_pool
	PacketHeader *packet_header = reinterpret_cast<PacketHeader *>(dest_buffer->getData());
	packet_header->ver = SETH_PACKET_HEADER_VERSION_V1;
	packet_header->opt_len = opt_len;
	packet_header->reserved = 0;
	packet_header->critical = 0;
	packet_header->oam = 0;
	packet_header->format = PacketHeaderFormat::ENCAPSULATED;
	packet_header->channel = 0;
	packet_header->sequence = seth_cpu_to_be_32(sequence++);
	// Dump the header into the destination buffer
	LOG_DEBUG_INTERNAL("  - FLUSH: ADDING HEADER: opts=", static_cast<unsigned int>(packet_header->opt_len));
	LOG_DEBUG_INTERNAL("  - FLUSH: DEST BUFFER SIZE: ", dest_buffer->getDataSize());

	// Flush buffer to the destination pool
	dest_buffer_pool->push(std::move(dest_buffer));

	// Grab a new buffer
	_getDestBuffer();
}

/**
 * @brief Get another buffer and prepare it for encoded data.
 *
 */
void PacketEncoder::_getDestBuffer() {
	// Grab a buffer to use for the resulting packet
	dest_buffer = buffer_pool->pop();
	dest_buffer->clear();
	// Advanced past header...
	dest_buffer->setDataSize(sizeof(PacketHeader));
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
	if (!dest_buffer->getDataSize()) {
		max_payload_size -= sizeof(PacketHeader);
	}
	// Each packet needs a packet header
	max_payload_size -= sizeof(PacketHeaderOption);

	// Next we add the current buffer size, as this is overhead we've already incurred
	max_payload_size -= dest_buffer->getDataSize();

	// Intermdiate check if we've got no space
	if (max_payload_size <= 0) {
		return 0;
	}

	// If the packet size would exceed the current
	if (size > max_payload_size) {
		max_payload_size -= sizeof(PacketHeaderOptionPartialData);
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
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Flusing inflight buffers: pool=", buffer_pool->getBufferCount(),
					   ", count=", inflight_buffers.size());
	// Free remaining buffers in flight
	if (!inflight_buffers.empty()) {
		buffer_pool->push(inflight_buffers);
	}
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Flusing inflight buffers after: pool=", buffer_pool->getBufferCount(),
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
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Packet added");
}

/**
 * @brief Construct a new Packet Encoder:: Packet Encoder object
 *
 * @param l2mtu Layer 2 MTU.
 * @param l4mtu Layer 4 MTU.
 * @param available_buffer_pool Available buffer pool.
 * @param destination_buffer_pool Destination buffer pool.
 */
PacketEncoder::PacketEncoder(uint16_t l2mtu, uint16_t l4mtu, accl::BufferPool<PacketBuffer> *available_buffer_pool,
							 accl::BufferPool<PacketBuffer> *destination_buffer_pool) {
	// As the constructor parameters have the same names as our data members, lets just use this-> for everything during init
	this->l2mtu = l2mtu;
	this->l4mtu = l4mtu;
	this->sequence = 1;

	// Setup buffer pools
	this->buffer_pool = available_buffer_pool;
	this->dest_buffer_pool = destination_buffer_pool;

	// Initialize the destination buffer for first use
	_getDestBuffer();
}

/**
 * @brief Destroy the Packet Encoder:: Packet Encoder object
 *
 */
PacketEncoder::~PacketEncoder() = default;

/**
 * @brief Encode a packet buffer.
 *
 * @param packetBuffer Packet buffer to encode.
 */
void PacketEncoder::encode(std::unique_ptr<PacketBuffer> packetBuffer) {
	LOG_DEBUG_INTERNAL("INCOMING PACKET: size=", packetBuffer->getDataSize(), " [l2mtu: ", l2mtu, ", l4mtu: ", l4mtu,
					   "], buffer_size=", dest_buffer->getDataSize(), ", packet_count=", packet_count, ", opt_len=", opt_len);

	// Make sure we cannot receive a packet that is greater than our MTU size
	if (packetBuffer->getDataSize() > l2mtu) {
		LOG_ERROR("Packet size ", packetBuffer->getDataSize(), " exceeds L2MTU size ", l2mtu, "?");
		// Free buffer to available pool
		buffer_pool->push(std::move(packetBuffer));
		return;
	}

	// Handle packets that are larger than the MSS
	if (packetBuffer->getDataSize() > _getMaxPayloadSize(packetBuffer->getDataSize())) {
		// Start off at packet part 1 and position 0...
		uint8_t part = 1;
		uint16_t packet_pos = 0; // Position in the packet we're stuffing into the encap packet

		LOG_DEBUG_INTERNAL(" - ENCODE PARTIAL PACKET - ");

		// Loop while we still have bits of the packet to stuff into the encap packets
		while (packet_pos < packetBuffer->getDataSize()) {
			// Amount of data left in the packet still to encode
			uint16_t packet_left = packetBuffer->getDataSize() - packet_pos;
			// Maximum payload size for the current encap packet
			uint16_t max_payload_size = _getMaxPayloadSize(packet_left);
			// Work out how much of this packet we're going to stuff into the encap packet
			uint16_t part_size = (packet_left > max_payload_size) ? max_payload_size : packet_left;

			LOG_DEBUG_INTERNAL("  - PARTIAL PACKET: max_payload_size=", max_payload_size,
							   ", part=", static_cast<unsigned int>(part), ", packet_pos=", packet_pos, ", part_size=", part_size);

			// Grab current buffer size so we can do some magic memory meddling below
			size_t cur_buffer_size = dest_buffer->getDataSize();

			PacketHeaderOption *packet_header_option =
				reinterpret_cast<PacketHeaderOption *>(dest_buffer->getData() + cur_buffer_size);
			packet_header_option->reserved = 0;
			packet_header_option->type = PacketHeaderOptionType::PARTIAL_PACKET;
			packet_header_option->packet_size = seth_cpu_to_be_16(packetBuffer->getDataSize());

			// Add packet header option
			cur_buffer_size += sizeof(PacketHeaderOption);
			// Only bump option len if this is not a subsequent packet header
			if (!packet_count)
				opt_len++;

			PacketHeaderOptionPartialData *packet_header_option_partial =
				reinterpret_cast<PacketHeaderOptionPartialData *>(dest_buffer->getData() + cur_buffer_size);
			packet_header_option_partial->payload_length = seth_cpu_to_be_16(part_size);
			packet_header_option_partial->part = part;
			packet_header_option_partial->reserved = 0;

			// Add packet header option for partial packets
			cur_buffer_size += sizeof(PacketHeaderOptionPartialData);

			// Once we're done messing with the buffer, set its size
			dest_buffer->setDataSize(cur_buffer_size);

			// Dump more of the packet into our destination buffer
			dest_buffer->append(packetBuffer->getData() + packet_pos, part_size);

			LOG_DEBUG_INTERNAL("    - Current dest buffer size: ", dest_buffer->getDataSize());

			// If the buffer is full, push it to the destination pool
			if (dest_buffer->getDataSize() == l4mtu) {
				LOG_DEBUG_INTERNAL("    - Buffer full, flushing");
				_flush();
			}

			// Adjust packet position
			packet_pos += part_size;
			part++;
		}

		// Push buffer into inflight list
		_pushInflight(packetBuffer);
		_flushInflight();

		// Bump packet count
		packet_count++;

		LOG_DEBUG_INTERNAL("  - Final dest buffer size is ", dest_buffer->getDataSize(), " (packets: ", packet_count, ")");

	} else {
		// Grab current buffer size so we can do some magic memory meddling below
		size_t cur_buffer_size = dest_buffer->getDataSize();

		LOG_DEBUG_INTERNAL(" - ENCODE COMPLETE PACKET - ");

		PacketHeaderOption *packet_header_option = reinterpret_cast<PacketHeaderOption *>(dest_buffer->getData() + cur_buffer_size);
		packet_header_option->reserved = 0;
		packet_header_option->type = PacketHeaderOptionType::COMPLETE_PACKET;
		packet_header_option->packet_size = seth_cpu_to_be_16(packetBuffer->getDataSize());

		LOG_DEBUG_INTERNAL("  - OPTION HEADER: packet_bufer_size=", packetBuffer->getDataSize(),
						   ", max_payload_size=", _getMaxPayloadSize(packetBuffer->getDataSize()),
						   ", header_option_size=", sizeof(PacketHeaderOption), ", dest_buffer_size=", cur_buffer_size);

		// Add packet header option
		cur_buffer_size += sizeof(PacketHeaderOption);
		// Only bump option len if this is not a subsequent packet header
		if (!packet_count)
			opt_len++;

		// Once we're done messing with the buffer, set its size
		dest_buffer->setDataSize(cur_buffer_size);

		dest_buffer->append(packetBuffer->getData(), packetBuffer->getDataSize());
		// Set packet buffer to being inflight
		_pushInflight(packetBuffer);

		// Bump packet count
		packet_count++;

		LOG_DEBUG_INTERNAL("  - FINAL DEST BUFFER SIZE: ", dest_buffer->getDataSize(), " (packets: ", packet_count, ")");
	}

	// If the buffer is full, push it to the destination pool
	// NK: We need to make provision for a header
	LOG_DEBUG_INTERNAL("   - Flush check: ", _getMaxPayloadSize(SETH_PACKET_MAX_SIZE));
	if (_getMaxPayloadSize(SETH_PACKET_MAX_SIZE) < sizeof(PacketHeader) + (sizeof(PacketHeaderOption) * 10)) {
		// Packet is as full as it will get
		_flush();
		_flushInflight();
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
