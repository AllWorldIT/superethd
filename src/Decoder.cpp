/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "Decoder.hpp"
#include "libaccl/Logger.hpp"
#include "util.hpp"
#include <cassert>

/*
 * Packet decoder
 */

void PacketDecoder::_clearState() {
	// Clear destination buffer
	dest_buffer->clear();
	// Now for the packet parts we get, clear the info for them too
	last_part = 0;
	last_final_packet_size = 0;
}

void PacketDecoder::_getDestBuffer() {
	// Grab a buffer to use for the resulting packet
	dest_buffer = buffer_pool->pop();
	_clearState();
}

void PacketDecoder::_flushInflight() {
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Flusing inflight buffers: pool=", buffer_pool->getBufferCount(),
					   ", count=", inflight_buffers.size());
	// Free buffers in flight
	if (!inflight_buffers.empty()) {
		buffer_pool->push(inflight_buffers);
	}
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Flusing inflight buffers after: pool=", buffer_pool->getBufferCount(),
					   ", count=", inflight_buffers.size());
}

void PacketDecoder::_pushInflight(std::unique_ptr<accl::Buffer> &packetBuffer) {
	// Push buffer into inflight list
	inflight_buffers.push_back(std::move(packetBuffer));
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Packet added");
}

void PacketDecoder::_clearStateAndFlushInflight(std::unique_ptr<accl::Buffer> &packetBuffer) {
	// Clear current state
	_clearState();
	// Flush inflight buffers
	_pushInflight(packetBuffer);
	_flushInflight();
}

PacketDecoder::PacketDecoder(uint16_t l2mtu, accl::BufferPool *available_buffer_pool, accl::BufferPool *destination_buffer_pool) {
	// As the constructor parameters have the same names as our data members, lets just use this-> for everything during init
	this->l2mtu = l2mtu;
	this->first_packet = true;

	// Setup buffer pools
	this->buffer_pool = available_buffer_pool;
	this->dest_buffer_pool = destination_buffer_pool;

	// Initialize the destination buffer for first use
	_getDestBuffer();
}

PacketDecoder::~PacketDecoder() = default;

void PacketDecoder::decode(std::unique_ptr<accl::Buffer> packetBuffer) {
	LOG_DEBUG_INTERNAL("DECODER INCOMING PACKET SIZE: ", packetBuffer->getDataSize());

	// Make sure packet is big enough to contain our header
	if (packetBuffer->getDataSize() < sizeof(PacketHeader)) {
		LOG_ERROR("Packet too small, should be > ", sizeof(PacketHeader));
		buffer_pool->push(std::move(packetBuffer));
		dest_buffer->clear();
		return;
	}

	// Overlay packet head onto packet
	// NK: we need to use a C-style cast here as reinterpret_cast casts against a single byte
	PacketHeader *packet_header = (PacketHeader *)packetBuffer->getData();

	// Check version is supported
	if (packet_header->ver > SETH_PACKET_HEADER_VERSION_V1) {
		LOG_ERROR("Packet not supported, version ", std::format("{:02X}", static_cast<unsigned int>(packet_header->ver)),
				  " vs.our version ", std::format("{:02X}", SETH_PACKET_HEADER_VERSION_V1), ", DROPPING !");
		// Clear current state and flush inflight buffers
		_clearStateAndFlushInflight(packetBuffer);
		return;
	}
	if (packet_header->reserved != 0) {
		LOG_ERROR("Packet header should not have any reserved its set, it is ",
				  std::format("{:02X}", static_cast<unsigned int>(packet_header->reserved)), ", DROPPING!");
		// Clear current state and flush inflight buffers
		_clearStateAndFlushInflight(packetBuffer);
		return;
	}
	// First thing we do is validate the header
	if (static_cast<uint8_t>(packet_header->format) &
		~(static_cast<uint8_t>(PacketHeaderFormat::ENCAPSULATED) | static_cast<uint8_t>(PacketHeaderFormat::COMPRESSED))) {
		LOG_ERROR("Packet in invalid format ", std::format("{:02X}", static_cast<unsigned int>(packet_header->format)),
				  ", DROPPING!");
		// Clear current state and flush inflight buffers
		_clearStateAndFlushInflight(packetBuffer);
		return;
	}
	// Next check the channel is set to 0
	if (packet_header->channel) {
		LOG_ERROR("Packet specifies invalid channel ", std::format("{:02X}", static_cast<unsigned int>(packet_header->channel)),
				  ", DROPPING!");
		// Clear current state and flush inflight buffers
		_clearStateAndFlushInflight(packetBuffer);
		return;
	}

	// Decode sequence
	uint32_t sequence = seth_be_to_cpu_32(packet_header->sequence);

	// If this is not the first packet...
	if (first_packet) {
		first_packet = false;
		last_sequence = sequence - 1;
	}

	// Check if we've lost packets
	if (sequence > last_sequence + 1) {
		LOG_INFO(sequence, ": Packet lost, last=", last_sequence, ", seq=", sequence, ", total_lost=", sequence - last_sequence);
		// Clear current state and flush inflight buffers
		_clearState();
		_flushInflight();
	}
	// If we we have an out of order one
	if (sequence < last_sequence + 1) {
		// Check that sequence hasn't wrapped
		if (is_sequence_wrapping(sequence, last_sequence)) {
			LOG_INFO(sequence, ": Sequence number probably wrapped: now=", sequence, ", prev=", last_sequence);
			last_sequence = 0;
		} else {
			LOG_NOTICE(sequence, ": PACKET OOO : last=", last_sequence, ", seq=", sequence);
			// Clear current state and flush inflight buffers
			_clearState();
			_flushInflight();
		}
	}
	// Set last sequence
	last_sequence = sequence;

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
			LOG_ERROR(sequence, ": Cannot read packet header option, buffer overrun");
			// Clear current state and flush inflight buffers
			_clearStateAndFlushInflight(packetBuffer);
			return;
		}
		// Overlay the option header
		PacketHeaderOption *packet_header_option = (PacketHeaderOption *)(packetBuffer->getData() + cur_pos);
		// Check for invalid header, where the reserved bits are set
		if (packet_header_option->reserved) {
			LOG_ERROR(sequence, ": Packet header option number ", static_cast<unsigned int>(header_num + 1),
					  " is invalid, reserved bits set");
			// Clear current state and flush inflight buffers
			_clearStateAndFlushInflight(packetBuffer);
			return;
		}
		// For now we just output the info
		LOG_DEBUG_INTERNAL(sequence, ":   - Packet header option: header=", static_cast<unsigned int>(header_num + 1),
						   ", type=", std::format("{:02X}", static_cast<unsigned int>(packet_header_option->type)));

		// If this header is a packet, we need to set packet_pos, data should immediately follow it
		if (!packet_header_pos &&
			static_cast<uint8_t>(packet_header_option->type) & (static_cast<uint8_t>(PacketHeaderOptionType::COMPLETE_PACKET) |
																static_cast<uint8_t>(PacketHeaderOptionType::PARTIAL_PACKET))) {
			LOG_DEBUG_INTERNAL(sequence, ":   - Found packet @", cur_pos);

			// Make sure our headers are in the correct positions, else something is wrong
			if (header_num + 1 != packet_header->opt_len) {
				LOG_ERROR(sequence,
						  ": Packet header should be the last header, current=", static_cast<unsigned int>(header_num + 1),
						  ", opt_len=", static_cast<unsigned int>(packet_header->opt_len));
				// Clear current state and flush inflight buffers
				_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// We need to  verify that the header option for partial packets can fit the payload in the available buffer
			if (packet_header_option->type == PacketHeaderOptionType::PARTIAL_PACKET) {

				// Make sure this option header can be read in the remaining data
				if (static_cast<ssize_t>(sizeof(PacketHeaderOption) + sizeof(PacketHeaderOptionPartialData)) >
					packetBuffer->getDataSize() - cur_pos) {
					LOG_ERROR(sequence, ": Cannot read packet header option partial data, buffer overrun");
					// Clear current state and flush inflight buffers
					_clearStateAndFlushInflight(packetBuffer);
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
		LOG_ERROR(sequence, ": Packet has invalid format", sequence);
		// Clear current state and flush inflight buffers
		_clearStateAndFlushInflight(packetBuffer);
		return;
	}

	// If we don't have a packet header, something is wrong
	if (!packet_header_pos) {
		LOG_ERROR(sequence, ": No packet header found, size=", packetBuffer->getDataSize());
		UT_ASSERT(packet_header_pos);
		// Clear current state and flush inflight buffers
		_clearStateAndFlushInflight(packetBuffer);
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
			LOG_ERROR(sequence, ": Packet too big for interface L2MTU, ", final_packet_size, " > ", l2mtu);
			// Clear current state and flush inflight buffers
			_clearStateAndFlushInflight(packetBuffer);
			return;
		}

		// When a packet is completely inside the encap packet
		if (packet_header_option->type == PacketHeaderOptionType::COMPLETE_PACKET) {
			uint16_t packet_pos = packet_header_pos + sizeof(PacketHeaderOption);

			LOG_DEBUG_INTERNAL(sequence, ":  - DECODE IS COMPLETE PACKET -");

			// Flush inflight if this is a complete packet
			flush_inflight = true;

			// Check if we had a last part?
			if (last_part) {
				LOG_NOTICE(sequence, ": At some stage we had a last part, but something got lost? clearing state");
				// Clear current state and flush inflight
				_clearState();
				_flushInflight();
			}

			// Make sure that our encapsulated packet doesnt exceed the packet size
			if (packet_pos + final_packet_size > packetBuffer->getDataSize()) {
				LOG_ERROR(sequence, ": Encapsulated packet size ", static_cast<uint16_t>(final_packet_size),
						  " would exceed encapsulating packet size ", packetBuffer->getDataSize(), " at offset ", packet_pos);
				// Clear current state and flush inflight buffers
				_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Append packet to dest buffer
			LOG_DEBUG_INTERNAL(sequence, ": Copy packet from pos ", packet_pos, " with size ", final_packet_size,
							   " into dest_buffer at position ", dest_buffer->getDataSize());
			dest_buffer->append(packetBuffer->getData() + packet_pos, final_packet_size);

			// Buffer ready, push to destination pool
			dest_buffer_pool->push(std::move(dest_buffer));

			// We're now outsie the path of direct IO, so get a new buffer here so we can be ready for when we're in the
			// direct IO path
			_getDestBuffer();

			// Bump the next packet header pos by the packet_pos plus the final_packet_size
			packet_header_pos = packet_pos + final_packet_size;

			// when a packet is split between encap packets
		} else if (packet_header_option->type == PacketHeaderOptionType::PARTIAL_PACKET) {
			LOG_DEBUG_INTERNAL(sequence, ":  - DECODE IS PARIAL PACKET -");

			// Do not flush inflight if this is a partial packet
			flush_inflight = false;
			// Are we going to skip this packet? we do this if we lose something along the way and cannot rebuild it
			bool skip_packet = false;

			// Packet header option partial data
			PacketHeaderOptionPartialData *packet_header_option_partial_data =
				(PacketHeaderOptionPartialData *)(packetBuffer->getData() + packet_header_pos + sizeof(PacketHeaderOption));

			// If our part is 0, it means we probably lost something if we had a last_part, we can clear the buffer and reset
			// last_part
			if (packet_header_option_partial_data->part == 1 && last_part) {
				LOG_NOTICE(sequence,
						   ": Something got lost, header_part=", static_cast<unsigned int>(packet_header_option_partial_data->part),
						   ", last_part=", static_cast<unsigned int>(last_part));
				// Clear current state and flush inflight
				_clearState();
				_flushInflight();
				skip_packet = true;

				// Verify part number
			} else if (packet_header_option_partial_data->part != last_part + 1) {
				LOG_NOTICE(sequence, ": Partial payload part ", static_cast<unsigned int>(packet_header_option_partial_data->part),
						   " does not match last_part=", static_cast<unsigned int>(last_part), " + 1, SKIPPING!");
				// Clear current state and flush inflight
				_clearState();
				_flushInflight();
				// Skip this packet
				skip_packet = true;

				// Verify packet size is the same as the last one
			} else if (last_part && final_packet_size != last_final_packet_size) {
				LOG_NOTICE(sequence, ": This final_packet_size=", final_packet_size,
						   " does not match last_final_packet_size=", last_final_packet_size, ", SKIPPING!");
				// Clear current state and flush inflight
				_clearState();
				_flushInflight();
				// Skip this packet
				skip_packet = true;
			}

			uint16_t packet_pos = packet_header_pos + sizeof(PacketHeaderOption) + sizeof(PacketHeaderOptionPartialData);
			uint16_t payload_length = seth_be_to_cpu_16(packet_header_option_partial_data->payload_length);

			// If we're not skipping the packet, lets throw it into the dest_buffer
			if (skip_packet) {
				LOG_DEBUG(sequence, ": Skipping unusable partial packet");
				goto bump_header_pos;
			}

			// Make sure that our encapsulated packet doesnt exceed the packet size
			if (packet_pos + payload_length > packetBuffer->getDataSize()) {
				LOG_ERROR(sequence, ": Encapsulated partial packet payload length ", payload_length,
						  " would exceed encapsulating packet size ", packetBuffer->getDataSize(), " at offset ", packet_pos);
				// Clear current state and flush inflight buffers
				_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Make sure the payload will fit into the destination buffer
			if (dest_buffer->getDataSize() + payload_length > dest_buffer->getBufferSize()) {
				LOG_ERROR(sequence, ": Partial payload length of ", payload_length, " plus current buffer data size of ",
						  dest_buffer->getDataSize(), " would exceed buffer size ", dest_buffer->getBufferSize());
				// Clear current state and flush inflight buffers
				_clearStateAndFlushInflight(packetBuffer);
				return;
			}
			// Check if the current buffer plus the payload we got now exceeds what the final packet size should be
			if (dest_buffer->getDataSize() + payload_length > final_packet_size) {
				LOG_ERROR(sequence, ": This should never happen, our packet dest_buffer_size=", dest_buffer->getDataSize(),
						  " + payload_length=", payload_length, " is bigger than the packet buffer of ",
						  packetBuffer->getDataSize(), ", DROPPING!!!");
				// Clear current state and flush inflight buffers
				_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Append packet to dest buffer
			LOG_DEBUG_INTERNAL(sequence, " Copy packet from pos ", packet_pos, " with size ", payload_length,
							   " into dest_buffer at position ", dest_buffer->getDataSize());
			dest_buffer->append(packetBuffer->getData() + packet_pos, payload_length);

			if (dest_buffer->getDataSize() == final_packet_size) {
				LOG_DEBUG_INTERNAL(sequence, ":   - Entire packet read... dumping into dest_buffer_pool & flushing inflight");
				// Buffer ready, push to destination pool
				dest_buffer_pool->push(std::move(dest_buffer));
				// We're now outsie the path of direct IO (maybe), so get a new buffer here so we can be ready for when we're in
				// the direct IO path
				_getDestBuffer();
				// We always flush inflight buffers when we have assembled a partial packet
				_flushInflight();
			} else {
				LOG_DEBUG_INTERNAL(sequence, ":   - Packet not entirely read, we need more");
				// Bump last part and set last_final_packet_size
				last_part = packet_header_option_partial_data->part;
				last_final_packet_size = final_packet_size;
			}

		bump_header_pos:
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
