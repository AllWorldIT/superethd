/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "Codec.hpp"
#include "Endian.hpp"
#include "debug.hpp"
#include "libaccl/BufferPool.hpp"
#include "libsethnetkit/EthernetPacket.hpp"
#include <cstring>
#include <format>
#include <iomanip>
#include <memory>

/*
 * Packet encoder
 */

void PacketEncoder::_getDestBuffer() {
	// Grab a buffer to use for the resulting packet
	dest_buffer = avail_buffer_pool->pop();
	dest_buffer->clear();
	// Advanced past header...
	dest_buffer->setDataSize(sizeof(PacketHeader));
	// Clear opt_len
	opt_len = 0;
}

PacketEncoder::PacketEncoder(uint16_t mss, uint16_t mtu, accl::BufferPool *available_buffer_pool,
							 accl::BufferPool *destination_buffer_pool) {
	// As the constructor parameters have the same names as our data members, lets just use this-> for everything during init
	this->mss = mss;
	this->mtu = mtu;
	this->sequence = 1;

	// Setup buffer pools
	this->avail_buffer_pool = available_buffer_pool;
	this->dest_buffer_pool = destination_buffer_pool;

	// Initialize the destination buffer for first use
	_getDestBuffer();
}

PacketEncoder::~PacketEncoder() = default;

void PacketEncoder::encode(const char *packet, uint16_t size) {
	DEBUG_CERR("INCOMING PACKET: size={} [mtu: {}, mss: {}], buffer_size={}", size, mtu, mss, dest_buffer->getDataSize());

	// Make sure we cannot receive a packet that is greater than our MTU size
	if (size > mtu + SETH_PACKET_ETHERNET_HEADER_LEN) {
		CERR("Packet size {} exceeds MTU size {}?", size, mtu);
		return;
	}

	// Handle packets that are larger than the MSS
	if (size > mss) {
		// Start off at packet part 1 and position 0...
		uint8_t part = 1;
		uint16_t packet_pos = 0;
		uint16_t max_payload_size = mss - sizeof(PacketHeader) - sizeof(PacketHeaderOption) - sizeof(PacketHeaderOptionPartialData);

		// Loop while we still have bits of the packet to stuff into the encap packets
		while (packet_pos < size) {
			// Work out how much of this packet we're going to stuff into the encap packet
			uint16_t part_size = (size - packet_pos > max_payload_size) ? max_payload_size : (size - packet_pos);

			DEBUG_CERR("PARTIAL PACKET: part={}, packet_pos={}, part_size={}", part, packet_pos, part_size);

			// Grab current buffer size so we can do some magic memory meddling below
			size_t cur_buffer_size = dest_buffer->getDataSize();

			PacketHeaderOption *packet_header_option =
				reinterpret_cast<PacketHeaderOption *>(dest_buffer->getData() + cur_buffer_size);
			packet_header_option->reserved = 0;
			packet_header_option->type = PacketHeaderOptionType::PARTIAL_PACKET;
			packet_header_option->packet_size = seth_cpu_to_be_16(size);

			DEBUG_CERR("OPTION HEADER SIZE: {}", sizeof(PacketHeaderOption));

			// Add packet header option
			cur_buffer_size += sizeof(PacketHeaderOption);
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
			dest_buffer->append(packet + packet_pos, part_size);

			DEBUG_CERR("FINAL DEST BUFFER SIZE: {}", dest_buffer->getDataSize());

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
			DEBUG_CERR("ADDING HEADER: opts={}", static_cast<uint8_t>(packet_header->opt_len));

			DEBUG_CERR("DEST BUFFER SIZE: {}", dest_buffer->getDataSize());

			// Buffer ready, push to destination pool
			dest_buffer_pool->push(dest_buffer);
			// We're now outsie the path of direct IO (maybe), so get a new buffer here so we can be ready for when we're in the
			// direct IO path
			_getDestBuffer();

			// Adjust packet position
			packet_pos += part_size;
			part++;
		}

	} else {

		// Grab current buffer size so we can do some magic memory meddling below
		size_t cur_buffer_size = dest_buffer->getDataSize();

		PacketHeaderOption *packet_header_option = reinterpret_cast<PacketHeaderOption *>(dest_buffer->getData() + cur_buffer_size);
		packet_header_option->reserved = 0;
		packet_header_option->type = PacketHeaderOptionType::COMPLETE_PACKET;
		packet_header_option->packet_size = seth_cpu_to_be_16(size);

		DEBUG_CERR("OPTION HEADER: cur_buffer_size={}, header_option_size={}", cur_buffer_size, sizeof(PacketHeaderOption));

		// Add packet header option
		cur_buffer_size += sizeof(PacketHeaderOption);
		opt_len++;

		// Once we're done messing with the buffer, set its size
		dest_buffer->setDataSize(cur_buffer_size);

		dest_buffer->append(packet, size);

		DEBUG_CERR("FINAL DEST BUFFER SIZE: {}", dest_buffer->getDataSize());

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
		DEBUG_CERR("ADDING HEADER: opts={}", static_cast<uint8_t>(packet_header->opt_len));

		DEBUG_CERR("DEST BUFFER SIZE: {}", dest_buffer->getDataSize());

		// Buffer ready, push to destination pool
		dest_buffer_pool->push(dest_buffer);
		// We're now outsie the path of direct IO (maybe), so get a new buffer here so we can be ready for when we're in the direct
		// IO path
		_getDestBuffer();
	}
}

/*
 * Packet decoder
 */

void PacketDecoder::_clearState() { // Clear destination buffer
	dest_buffer->clear();
}

void PacketDecoder::_getDestBuffer() {
	// Grab a buffer to use for the resulting packet
	dest_buffer = avail_buffer_pool->pop();
	_clearState();
}

PacketDecoder::PacketDecoder(uint16_t mtu, accl::BufferPool *available_buffer_pool, accl::BufferPool *destination_buffer_pool) {
	// As the constructor parameters have the same names as our data members, lets just use this-> for everything during init
	this->mtu = mtu;
	this->first_packet = true;
	this->last_sequence = 0;

	// Setup buffer pools
	this->avail_buffer_pool = available_buffer_pool;
	this->dest_buffer_pool = destination_buffer_pool;

	// Initialize the destination buffer for first use
	_getDestBuffer();
}

PacketDecoder::~PacketDecoder() = default;

void PacketDecoder::decode(const char *packet, uint16_t size) {
	DEBUG_CERR("DECODER INCOMING PACKET SIZE: {}", size);

	// Make sure packet is big enough to contain our header
	if (size < sizeof(PacketHeader)) {
		CERR("Packet too small, should be > {}", sizeof(PacketHeader));
		dest_buffer->clear();
		return;
	}

	// Overlay packet head onto packet
	// NK: we need to use a C-style cast here as reinterpret_cast casts against a single byte
	PacketHeader *packet_header = (PacketHeader *)packet;

	// Check version is supported
	if (packet_header->ver > SETH_PACKET_HEADER_VERSION_V1) {
		DEBUG_CERR("PACKET VERSION: {:02X}", static_cast<uint8_t>(packet_header->ver));
		CERR("Packet not supported, version {:02X} vs. our version {:02X}, DROPPING!", static_cast<uint8_t>(packet_header->ver),
			 SETH_PACKET_HEADER_VERSION_V1);
		// Clear current state
		_clearState();
		return;
	}
	if (packet_header->reserved != 0) {
		CERR("Packet header should not have any reserved its set, it is {:02X}, DROPPING!",
			 static_cast<uint8_t>(packet_header->reserved));
		// Clear current state
		_clearState();
		return;
	}
	// First thing we do is validate the header
	if (static_cast<uint8_t>(packet_header->format) &
		~(static_cast<uint8_t>(PacketHeaderFormat::ENCAPSULATED) | static_cast<uint8_t>(PacketHeaderFormat::COMPRESSED))) {
		CERR("Packet in invalid format {:02X}, DROPPING!", static_cast<uint8_t>(packet_header->format));
		// Clear current state
		_clearState();
		return;
	}
	// Next check the channel is set to 0
	if (packet_header->channel) {
		CERR("Packet specifies invalid channel {:02X}, DROPPING!", static_cast<uint8_t>(packet_header->channel));
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
			// Clear current state
			_clearState();
		}
		// If we we have an out of order one
		if (sequence < last_sequence + 1) {
			CERR("PACKET OOO : last={}, seq={}", last_sequence, sequence);
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
		if (static_cast<ssize_t>(sizeof(PacketHeaderOption)) > size - cur_pos) {
			CERR("ERROR: Cannot read packet header option, buffer overrun");
			// Clear current state
			_clearState();
			return;
		}
		// Overlay the option header
		PacketHeaderOption *packet_header_option = (PacketHeaderOption *)(packet + cur_pos);
		// Check for invalid header, where the reserved bits are set
		if (packet_header_option->reserved) {
			CERR("ERROR: Packet header option number {} is invalid, reserved bits set", header_num + 1);
			// Clear current state
			_clearState();
			return;
		}
		// For now we just output the info
		CERR("  - Packet header option: header={}, type={:02X}", header_num + 1, static_cast<uint8_t>(packet_header_option->type));

		// If this header is a packet, we need to set packet_pos, data should immediately follow it
		if (!packet_header_pos &&
			static_cast<uint8_t>(packet_header_option->type) & (static_cast<uint8_t>(PacketHeaderOptionType::COMPLETE_PACKET) |
																static_cast<uint8_t>(PacketHeaderOptionType::PARTIAL_PACKET))) {
			CERR("  - Found packet @{}", cur_pos);

			// Make sure our headers are in the correct positions, else something is wrong
			if (header_num + 1 != packet_header->opt_len) {
				CERR("ERROR: Packet header should be the last header, current={}, opt_len={}", header_num + 1,
					 static_cast<uint8_t>(packet_header->opt_len));
				// Clear current state
				_clearState();
				return;
			}

			// We need to  verify that the header option for partial packets can fit the payload in the available buffer
			if (packet_header_option->type == PacketHeaderOptionType::PARTIAL_PACKET) {

				// Make sure this option header can be read in the remaining data
				if (static_cast<ssize_t>(sizeof(PacketHeaderOption) + sizeof(PacketHeaderOptionPartialData)) > size - cur_pos) {
					CERR("ERROR: Cannot read packet header option partial data, buffer overrun");
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

	if (packet_header->format == PacketHeaderFormat::ENCAPSULATED) {

		// If we don't have a packet header, something is wrong
		if (!packet_header_pos) {
			CERR("No packet found");
			// Clear current state
			_clearState();
			return;
		}

		// Packet header option
		PacketHeaderOption *packet_header_option = (PacketHeaderOption *)(packet + packet_header_pos);

		uint16_t encap_packet_size = seth_be_to_cpu_16(packet_header_option->packet_size);

		// When a packet is completely inside the encap packet
		if (packet_header_option->type == PacketHeaderOptionType::COMPLETE_PACKET) {
			uint16_t packet_pos = packet_header_pos + sizeof(PacketHeaderOption);

			// Make sure that our encapsulated packet doesnt exceed the packet size
			if (packet_pos + encap_packet_size > size) {
				CERR("ERROR: Encapsulated packet size {} would exceed encapsulating packet size {} at offset {}",
					 static_cast<uint16_t>(encap_packet_size), size, packet_pos);
				// Clear current state
				_clearState();
				return;
			}

			// Append packet to dest buffer
			DEBUG_CERR("Copy packet from pos {} with size {} into dest_buffer at position {}", packet_pos, encap_packet_size,
					   dest_buffer->getDataSize());
			dest_buffer->append(packet + packet_pos, encap_packet_size);

			// Buffer ready, push to destination pool
			dest_buffer_pool->push(dest_buffer);
			// We're now outsie the path of direct IO (maybe), so get a new buffer here so we can be ready for when we're in the
			// direct IO path
			_getDestBuffer();

			// FIXME - remove
			if (packet_pos + encap_packet_size < size) {
				DEBUG_CERR("WE NEED TO CHEW ON ANOTHER PACKET: packet_pos={}, encap_packet_size={}, size={}", packet_pos,
						   encap_packet_size, size);
			}

			// when a packet is split between encap packets
		} else if (packet_header_option->type == PacketHeaderOptionType::PARTIAL_PACKET) {

			// Packet header option partial data
			PacketHeaderOptionPartialData *packet_header_option_partial_data =
				(PacketHeaderOptionPartialData *)(packet + packet_header_pos + sizeof(PacketHeaderOption));

			uint16_t packet_pos = packet_header_pos + sizeof(PacketHeaderOption) + sizeof(PacketHeaderOptionPartialData);
			uint16_t payload_length = seth_be_to_cpu_16(packet_header_option_partial_data->payload_length);

			// Make sure that our encapsulated packet doesnt exceed the packet size
			if (packet_pos + payload_length > size) {
				CERR("ERROR: Encapsulated partial packet payload length {} would exceed encapsulating packet size {} at offset {}",
					 payload_length, size, packet_pos);
				// Clear current state
				_clearState();
				return;
			}

			// Make sure the payload will fit into the destination buffer
			if (dest_buffer->getDataSize() + payload_length > dest_buffer->getBufferSize()) {
				CERR("ERROR: Partial payload length of {} plus current buffer data size of {} would exceed buffer size {}",
					 payload_length, dest_buffer->getDataSize(), dest_buffer->getBufferSize());
				// Clear current state
				_clearState();
				return;
			}

			// TODO: Check packet sequence
			// TODO: Check packet part number

			// Append packet to dest buffer
			DEBUG_CERR("Copy packet from pos {} with size {} into dest_buffer at position {}", packet_pos, payload_length,
					   dest_buffer->getDataSize());
			dest_buffer->append(packet + packet_pos, payload_length);

			// Check if we have the entire packet
			if (dest_buffer->getDataSize() == encap_packet_size) {
				DEBUG_CERR("  - Entire packet read... dumping into dest_buffer_pool")
				// Buffer ready, push to destination pool
				dest_buffer_pool->push(dest_buffer);
				// We're now outsie the path of direct IO (maybe), so get a new buffer here so we can be ready for when we're in the
				// direct IO path
				_getDestBuffer();
			} else {
				DEBUG_CERR("  - Packet not entirely read, we need more")
			}

			// FIXME - remove
			if (packet_pos + encap_packet_size < size) {
				DEBUG_CERR("WE NEED TO CHEW ON ANOTHER PACKET: packet_pos={}, encap_packet_size={}, size={}", packet_pos,
						   encap_packet_size, size);
			}
		}
	}
}
