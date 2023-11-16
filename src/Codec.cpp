/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "Codec.hpp"
#include "Endian.hpp"
#include "libaccl/BufferPool.hpp"
#include "libaccl/Logger.hpp"
#include "libsethnetkit/EthernetPacket.hpp"
#include <catch2/internal/catch_console_colour.hpp>
#include <cstring>
#include <format>
#include <iomanip>
#include <memory>

/*
 * Packet encoder
 */

void PacketEncoder::_flush() {
	// If we don't have data in the buffer, just return
	if (!dest_buffer->getDataSize())
		return;

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
	LOG_DEBUG_INTERNAL("  - FLUSH: ADDING HEADER: opts={}", static_cast<uint8_t>(packet_header->opt_len));
	LOG_DEBUG_INTERNAL("  - FLUSH: DEST BUFFER SIZE: {}", dest_buffer->getDataSize());

	// Flush buffer to the destination pool
	dest_buffer_pool->push(dest_buffer);

	// Grab a new buffer
	_getDestBuffer();
}

void PacketEncoder::_getDestBuffer() {
	// Grab a buffer to use for the resulting packet
	dest_buffer = avail_buffer_pool->pop();
	dest_buffer->clear();
	// Advanced past header...
	dest_buffer->setDataSize(sizeof(PacketHeader));
	// Clear opt_len
	opt_len = 0;
	// Reset packet count to 0
	packet_count = 0;
}

void PacketEncoder::_flushInflight() {
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Flusing inflight buffers: count={}", inflight_buffers.size());
	// Free remaining buffers in flight
	if (!inflight_buffers.empty()) {
		avail_buffer_pool->push(inflight_buffers);
		inflight_buffers.clear();
	}
}

void PacketEncoder::_pushInflight(std::unique_ptr<accl::Buffer> &packetBuffer) {
	// Push buffer into inflight list
	inflight_buffers.push_back(std::move(packetBuffer));
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Packet added")
}

PacketEncoder::PacketEncoder(uint16_t l2mtu, uint16_t l4mtu, accl::BufferPool *available_buffer_pool,
							 accl::BufferPool *destination_buffer_pool) {
	// As the constructor parameters have the same names as our data members, lets just use this-> for everything during init
	this->l2mtu = l2mtu;
	this->l4mtu = l4mtu;
	this->sequence = 1;

	// Setup buffer pools
	this->avail_buffer_pool = available_buffer_pool;
	this->dest_buffer_pool = destination_buffer_pool;

	// Initialize the destination buffer for first use
	_getDestBuffer();
}

PacketEncoder::~PacketEncoder() = default;

void PacketEncoder::encode(std::unique_ptr<accl::Buffer> packetBuffer) {
	LOG_DEBUG_INTERNAL("INCOMING PACKET: size={} [l2mtu: {}, l4mtu: {}], buffer_size={}", packetBuffer->getDataSize(), l2mtu, l4mtu,
					   dest_buffer->getDataSize());

	// Make sure we cannot receive a packet that is greater than our MTU size
	if (packetBuffer->getDataSize() > l2mtu) {
		LOG_ERROR("Packet size {} exceeds L2MTU size {}?", packetBuffer->getDataSize(), l2mtu);
		// Free buffer to available pool
		avail_buffer_pool->push(packetBuffer);
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

			LOG_DEBUG_INTERNAL("  - PARTIAL PACKET: max_payload_size={}, part={}, packet_pos={}, part_size={}", max_payload_size,
							   part, packet_pos, part_size);

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

			LOG_DEBUG_INTERNAL("    - Current dest buffer size: {}", dest_buffer->getDataSize());

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

		LOG_DEBUG_INTERNAL("  - Final dest buffer size is {} (packets: {})", dest_buffer->getDataSize(), packet_count);

	} else {
		// Grab current buffer size so we can do some magic memory meddling below
		size_t cur_buffer_size = dest_buffer->getDataSize();

		LOG_DEBUG_INTERNAL(" - ENCODE COMPLETE PACKET - ");

		PacketHeaderOption *packet_header_option = reinterpret_cast<PacketHeaderOption *>(dest_buffer->getData() + cur_buffer_size);
		packet_header_option->reserved = 0;
		packet_header_option->type = PacketHeaderOptionType::COMPLETE_PACKET;
		packet_header_option->packet_size = seth_cpu_to_be_16(packetBuffer->getDataSize());

		LOG_DEBUG_INTERNAL("  - OPTION HEADER: max_payload_size={}, cur_buffer_size={}, header_option_size={}",
						   _getMaxPayloadSize(packetBuffer->getDataSize()), cur_buffer_size, sizeof(PacketHeaderOption));

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

		LOG_DEBUG_INTERNAL("  - FINAL DEST BUFFER SIZE: {} (packets: {})", dest_buffer->getDataSize(), packet_count);
	}

	// If the buffer is full, push it to the destination pool
	// NK: We need to make provision for a header
	if (_getMaxPayloadSize(SETH_PACKET_MAX_SIZE) < sizeof(PacketHeader) + (sizeof(PacketHeaderOption) * 10)) {
		// Packet is as full as it will get
		_flush();
		_flushInflight();
	}
}

void PacketEncoder::flush() {
	// When we get a flush, it means we probably can't fill the buffer anymore, so we can just dump it and the inflight buffers too
	_flush();
	_flushInflight();
}

/*
 * Packet decoder
 */


void PacketDecoder::_clearState() {
	// Clear destination buffer
	dest_buffer->clear();
	// Clear last encap packet sequence number
	this->last_sequence = 0;
	// Now for the packet parts we get, clear the info for them too
	this->last_part = 0;
	this->last_final_packet_size = 0;
}

void PacketDecoder::_getDestBuffer() {
	// Grab a buffer to use for the resulting packet
	dest_buffer = avail_buffer_pool->pop();
	_clearState();
}

void PacketDecoder::_flushInflight() {
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Flusing inflight buffers: count={}", inflight_buffers.size());
	// Free buffers in flight
	if (!inflight_buffers.empty()) {
		avail_buffer_pool->push(inflight_buffers);
		inflight_buffers.clear();
	}
}

void PacketDecoder::_pushInflight(std::unique_ptr<accl::Buffer> &packetBuffer) {
	// Push buffer into inflight list
	inflight_buffers.push_back(std::move(packetBuffer));
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Packet added")
}

PacketDecoder::PacketDecoder(uint16_t l2mtu, accl::BufferPool *available_buffer_pool, accl::BufferPool *destination_buffer_pool) {
	// As the constructor parameters have the same names as our data members, lets just use this-> for everything during init
	this->l2mtu = l2mtu;
	this->first_packet = true;

	// Setup buffer pools
	this->avail_buffer_pool = available_buffer_pool;
	this->dest_buffer_pool = destination_buffer_pool;

	// Initialize the destination buffer for first use
	_getDestBuffer();
}

PacketDecoder::~PacketDecoder() = default;

void PacketDecoder::decode(std::unique_ptr<accl::Buffer> packetBuffer) {
	LOG_DEBUG_INTERNAL("DECODER INCOMING PACKET SIZE: {}", packetBuffer->getDataSize());

	// Make sure packet is big enough to contain our header
	if (packetBuffer->getDataSize() < sizeof(PacketHeader)) {
		CERR("Packet too small, should be > {}", sizeof(PacketHeader));
		avail_buffer_pool->push(packetBuffer);
		dest_buffer->clear();
		return;
	}

	// Overlay packet head onto packet
	// NK: we need to use a C-style cast here as reinterpret_cast casts against a single byte
	PacketHeader *packet_header = (PacketHeader *)packetBuffer->getData();

	// Check version is supported
	if (packet_header->ver > SETH_PACKET_HEADER_VERSION_V1) {
		LOG_DEBUG_INTERNAL("PACKET VERSION: {:02X}", static_cast<uint8_t>(packet_header->ver));
		CERR("Packet not supported, version {:02X} vs. our version {:02X}, DROPPING!", static_cast<uint8_t>(packet_header->ver),
			 SETH_PACKET_HEADER_VERSION_V1);
		// Add packet to inflight list so its flushed to the available pool
		_pushInflight(packetBuffer);
		// Clear current state
		_clearState();
		return;
	}
	if (packet_header->reserved != 0) {
		CERR("Packet header should not have any reserved its set, it is {:02X}, DROPPING!",
			 static_cast<uint8_t>(packet_header->reserved));
		// Add packet to inflight list so its flushed to the available pool
		_pushInflight(packetBuffer);
		// Clear current state
		_clearState();
		return;
	}
	// First thing we do is validate the header
	if (static_cast<uint8_t>(packet_header->format) &
		~(static_cast<uint8_t>(PacketHeaderFormat::ENCAPSULATED) | static_cast<uint8_t>(PacketHeaderFormat::COMPRESSED))) {
		CERR("Packet in invalid format {:02X}, DROPPING!", static_cast<uint8_t>(packet_header->format));
		// Add packet to inflight list so its flushed to the available pool
		_pushInflight(packetBuffer);
		// Clear current state
		_clearState();
		return;
	}
	// Next check the channel is set to 0
	if (packet_header->channel) {
		CERR("Packet specifies invalid channel {:02X}, DROPPING!", static_cast<uint8_t>(packet_header->channel));
		// Add packet to inflight list so its flushed to the available pool
		_pushInflight(packetBuffer);
		// Clear current state
		_clearState();
		return;
	}

	// Decode sequence
	uint32_t sequence = seth_be_to_cpu_32(packet_header->sequence);

	// If this is not the first packet...
	if (!first_packet) {
		// Check if we've lost packets
		if (sequence > last_sequence + 1) {
			CERR("PACKET LOST: last={}, seq={}, total_lost={}", last_sequence, sequence, sequence - last_sequence);
			// Add packet to inflight list so its flushed to the available pool
			_pushInflight(packetBuffer);
			// Clear current state
			_clearState();
		}
		// If we we have an out of order one
		if (sequence < last_sequence + 1) {
			CERR("PACKET OOO : last={}, seq={}", last_sequence, sequence);
			// Add packet to inflight list so its flushed to the available pool
			_pushInflight(packetBuffer);
			// Clear current state
			_clearState();
		}
	}

	/*
	 * Process header options
	 */

	// Next we check the headers that follow and read in any processing options
	// NK: we are not processing the packets here, just reading in options
	uint16_t cur_pos = sizeof(PacketHeader); // Our first option header starts after our packet header
	uint16_t packet_header_pos = 0;
	for (uint8_t header_num = 0; header_num < packet_header->opt_len; ++header_num) {
		// Make sure this option header can be read in the remaining data
		if (static_cast<ssize_t>(sizeof(PacketHeaderOption)) > packetBuffer->getDataSize() - cur_pos) {
			CERR("ERROR: Cannot read packet header option, buffer overrun");
			// Add packet to inflight list so its flushed to the available pool
			_pushInflight(packetBuffer);
			// Clear current state
			_clearState();
			return;
		}
		// Overlay the option header
		PacketHeaderOption *packet_header_option = (PacketHeaderOption *)(packetBuffer->getData() + cur_pos);
		// Check for invalid header, where the reserved bits are set
		if (packet_header_option->reserved) {
			CERR("ERROR: Packet header option number {} is invalid, reserved bits set", header_num + 1);
			// Add packet to inflight list so its flushed to the available pool
			_pushInflight(packetBuffer);
			// Clear current state
			_clearState();
			return;
		}
		// For now we just output the info
		LOG_DEBUG_INTERNAL("  - Packet header option: header={}, type={:02X}", header_num + 1,
						   static_cast<uint8_t>(packet_header_option->type));

		// If this header is a packet, we need to set packet_pos, data should immediately follow it
		if (!packet_header_pos &&
			static_cast<uint8_t>(packet_header_option->type) & (static_cast<uint8_t>(PacketHeaderOptionType::COMPLETE_PACKET) |
																static_cast<uint8_t>(PacketHeaderOptionType::PARTIAL_PACKET))) {
			LOG_DEBUG_INTERNAL("  - Found packet @{}", cur_pos);

			// Make sure our headers are in the correct positions, else something is wrong
			if (header_num + 1 != packet_header->opt_len) {
				CERR("ERROR: Packet header should be the last header, current={}, opt_len={}", header_num + 1,
					 static_cast<uint8_t>(packet_header->opt_len));
				// Add packet to inflight list so its flushed to the available pool
				_pushInflight(packetBuffer);
				// Clear current state
				_clearState();
				return;
			}

			// We need to  verify that the header option for partial packets can fit the payload in the available buffer
			if (packet_header_option->type == PacketHeaderOptionType::PARTIAL_PACKET) {

				// Make sure this option header can be read in the remaining data
				if (static_cast<ssize_t>(sizeof(PacketHeaderOption) + sizeof(PacketHeaderOptionPartialData)) >
					packetBuffer->getDataSize() - cur_pos) {
					CERR("ERROR: Cannot read packet header option partial data, buffer overrun");
					// Add packet to inflight list so its flushed to the available pool
					_pushInflight(packetBuffer);
					// Clear current state
					_clearState();
					return;
				}
			}
			// Set packet header position
			packet_header_pos = cur_pos;
		}

		// Bump the pos past the header option
		cur_pos += sizeof(PacketHeaderOption);
		// Bump the pos depending on the type of header option
		if (packet_header_option->type == PacketHeaderOptionType::PARTIAL_PACKET) {
			cur_pos += sizeof(PacketHeaderOptionPartialData);
		}
	}

	/*
	 * Process packet
	 */

	if (packet_header->format != PacketHeaderFormat::ENCAPSULATED) {
		// Add packet to inflight list so its flushed to the available pool
		_pushInflight(packetBuffer);
		// Clear current state
		_clearState();
		return;
	}

	// If we don't have a packet header, something is wrong
	if (!packet_header_pos) {
		CERR("No packet found");
		// Add packet to inflight list so its flushed to the available pool
		_pushInflight(packetBuffer);
		// Clear current state
		_clearState();
		return;
	}

	// This flag indicates if we're going to be flushing the inflight buffers
	bool flush_inflight = true;

	// Loop while the packet_header_pos is not the end of the encap packet
	while (packet_header_pos < packetBuffer->getDataSize()) {

		// Packet header option
		PacketHeaderOption *packet_header_option = (PacketHeaderOption *)(packetBuffer->getData() + packet_header_pos);

		uint16_t final_packet_size = seth_be_to_cpu_16(packet_header_option->packet_size);

		// Make sure final packet size does not exceed MTU + ETHERNET_HEADER_LEN
		if (final_packet_size > l2mtu) {
			CERR("ERROR: Packet too big for interface L2MTU, {} > {}", final_packet_size, l2mtu);
			// Add packet to inflight list so its flushed to the available pool
			_pushInflight(packetBuffer);
			// Clear current state
			_clearState();
			return;
		}

		// When a packet is completely inside the encap packet
		if (packet_header_option->type == PacketHeaderOptionType::COMPLETE_PACKET) {
			uint16_t packet_pos = packet_header_pos + sizeof(PacketHeaderOption);

			LOG_DEBUG_INTERNAL(" - DECODE IS COMPLETE PACKET -")

			// Flush inflight if this is a complete packet
			flush_inflight = true;

			// Check if we had a last part?
			if (last_part) {
				CERR("At some stage we had a last part, but something got lost? clearing state");
				// Add packet to inflight list so its flushed to the available pool
				_pushInflight(packetBuffer);
				// Clear current state
				_clearState();
			}

			// Make sure that our encapsulated packet doesnt exceed the packet size
			if (packet_pos + final_packet_size > packetBuffer->getDataSize()) {
				CERR("ERROR: Encapsulated packet size {} would exceed encapsulating packet size {} at offset {}",
					 static_cast<uint16_t>(final_packet_size), packetBuffer->getDataSize(), packet_pos);
				// Add packet to inflight list so its flushed to the available pool
				_pushInflight(packetBuffer);
				// Clear current state
				_clearState();
				return;
			}

			// Append packet to dest buffer
			LOG_DEBUG_INTERNAL("Copy packet from pos {} with size {} into dest_buffer at position {}", packet_pos,
							   final_packet_size, dest_buffer->getDataSize());
			dest_buffer->append(packetBuffer->getData() + packet_pos, final_packet_size);

			// Buffer ready, push to destination pool
			dest_buffer_pool->push(dest_buffer);

			// We're now outsie the path of direct IO, so get a new buffer here so we can be ready for when we're in the
			// direct IO path
			_getDestBuffer();

			// Bump the next packet header pos by the packet_pos plus the final_packet_size
			packet_header_pos = packet_pos + final_packet_size;

			// when a packet is split between encap packets
		} else if (packet_header_option->type == PacketHeaderOptionType::PARTIAL_PACKET) {
			LOG_DEBUG_INTERNAL(" - DECODE IS PARIAL PACKET -")

			// Do not flush inflight if this is a partial packet
			flush_inflight = false;

			// Packet header option partial data
			PacketHeaderOptionPartialData *packet_header_option_partial_data =
				(PacketHeaderOptionPartialData *)(packetBuffer->getData() + packet_header_pos + sizeof(PacketHeaderOption));

			// If our part is 0, it means we probably lost something if we had a last_part, we can clear the buffer and reset
			// last_part
			if (packet_header_option_partial_data->part == 1 && last_part) {
				CERR("ERROR: Something got lost, we're dropping the current partial packet we have")
				// Add packet to inflight list so its flushed to the available pool
				_pushInflight(packetBuffer);
				// Clear current state
				_clearState();

			} else {
				// Verify part number
				if (packet_header_option_partial_data->part != last_part + 1) {
					CERR("ERROR: Partial payload part {} does not match last_part={} + 1", packet_header_option_partial_data->part,
						 last_part);
					// Add packet to inflight list so its flushed to the available pool
					_pushInflight(packetBuffer);
					// Clear current state
					_clearState();
					return;
				}
				// Verify packet size is the same as the last one
				if (last_part && final_packet_size != last_final_packet_size) {
					CERR("ERROR: This final_packet_size={} does not match last_final_packet_size={}", final_packet_size,
						 last_final_packet_size);
					// Add packet to inflight list so its flushed to the available pool
					_pushInflight(packetBuffer);
					// Clear current state
					_clearState();
					return;
				}
			}

			uint16_t packet_pos = packet_header_pos + sizeof(PacketHeaderOption) + sizeof(PacketHeaderOptionPartialData);
			uint16_t payload_length = seth_be_to_cpu_16(packet_header_option_partial_data->payload_length);

			// Make sure that our encapsulated packet doesnt exceed the packet size
			if (packet_pos + payload_length > packetBuffer->getDataSize()) {
				CERR("ERROR: Encapsulated partial packet payload length {} would exceed encapsulating packet size {} at offset "
					 "{}",
					 payload_length, packetBuffer->getDataSize(), packet_pos);
				// Add packet to inflight list so its flushed to the available pool
				_pushInflight(packetBuffer);
				// Clear current state
				_clearState();
				return;
			}

			// Make sure the payload will fit into the destination buffer
			if (dest_buffer->getDataSize() + payload_length > dest_buffer->getBufferSize()) {
				CERR("ERROR: Partial payload length of {} plus current buffer data size of {} would exceed buffer size {}",
					 payload_length, dest_buffer->getDataSize(), dest_buffer->getBufferSize());
				// Add packet to inflight list so its flushed to the available pool
				_pushInflight(packetBuffer);
				// Clear current state
				_clearState();
				return;
			}
			// Check if the current buffer plus the payload we got now exceeds what the final packet size should be
			if (dest_buffer->getDataSize() + payload_length > final_packet_size) {
				CERR("This should never happen, our packet getDataSize()={} + payload_length={} is bigger than the packet size "
					 "of {}, DROPPING!!!",
					 dest_buffer->getDataSize(), payload_length, packetBuffer->getDataSize());
				// Add packet to inflight list so its flushed to the available pool
				_pushInflight(packetBuffer);
				// Clear current state
				_clearState();
			}

			// Append packet to dest buffer
			LOG_DEBUG_INTERNAL("Copy packet from pos {} with size {} into dest_buffer at position {}", packet_pos, payload_length,
							   dest_buffer->getDataSize());
			dest_buffer->append(packetBuffer->getData() + packet_pos, payload_length);

			if (dest_buffer->getDataSize() == final_packet_size) {
				LOG_DEBUG_INTERNAL("  - Entire packet read... dumping into dest_buffer_pool & flushing inflight")
				// Buffer ready, push to destination pool
				dest_buffer_pool->push(dest_buffer);
				// We're now outsie the path of direct IO (maybe), so get a new buffer here so we can be ready for when we're in
				// the direct IO path
				_getDestBuffer();
				// We always flush inflight buffers when we have assembled a partial packet
				_flushInflight();
			} else {
				LOG_DEBUG_INTERNAL("  - Packet not entirely read, we need more")
				// Bump last part and set last_final_packet_size
				last_part = packet_header_option_partial_data->part;
				last_final_packet_size = final_packet_size;
			}

			// Bump the next packet header pos by the packet_pos plus the packet payload length
			packet_header_pos = packet_pos + payload_length;
		}
	}

	// Add packet to inflight list so its flushed to the available pool
	_pushInflight(packetBuffer);
	// Check if we're flushing the inflight buffers
	if (flush_inflight) {
		_flushInflight();
	}
}
