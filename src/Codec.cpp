/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "Codec.hpp"
#include "Endian.hpp"
#include "debug.hpp"
#include "libaccl/BufferPool.hpp"
#include <cstring>
#include <format>
#include <iomanip>

/*
 * Packet encoder
 */

void PacketEncoder::_getDestBuffer() {
	// Grab a buffer to use for the resulting packet
	dest_buffer = avail_buffer_pool->pop();
	dest_buffer->clear();
	// Advanced past header...
	dest_buffer->setDataSize(sizeof(PacketHeader));
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

// TODO: function to encode packet vector
//

void PacketEncoder::encode(const char *packet, uint16_t size) {
	DEBUG_CERR("INCOMING PACKET: size={}, buffer_size={}", size, dest_buffer->getDataSize());

	uint8_t opt_len = 0;

	PacketHeaderOption packet_header_option;
	packet_header_option.reserved = 0;
	packet_header_option.type = PacketPayloadHeaderType::COMPLETE_PACKET;
	packet_header_option.packet_size = size;

	DEBUG_CERR("OPTION HEADER SIZE: {}", sizeof(PacketHeaderOption));

	// Add packet payload header
	dest_buffer->append(reinterpret_cast<const char *>(&packet_header_option), sizeof(PacketHeaderOption));
	opt_len++;

	dest_buffer->append(packet, size);

	DEBUG_CERR("FINAL DEST BUFFER SIZE: {}", dest_buffer->getDataSize());

	// The first thing we're going to do is write our header
	PacketHeader packet_header;
	packet_header.ver = SETH_PACKET_HEADER_VERSION_V1;
	packet_header.opt_len = opt_len;
	packet_header.reserved = 0;
	packet_header.critical = 0;
	packet_header.oam = 0;
	packet_header.format = PacketHeaderFormat::ENCAPSULATED;
	packet_header.channel = 0;
	packet_header.sequence = seth_cpu_to_be_32(sequence++);
	// Dump the header into the destination buffer
	DEBUG_CERR("ADDING HEADER: opts={}", static_cast<uint8_t>(packet_header.opt_len));
	std::memcpy(dest_buffer->getData(), reinterpret_cast<const uint8_t *>(&packet_header), sizeof(PacketHeader));

	dest_buffer->append(reinterpret_cast<const char *>(&packet_header), sizeof(PacketHeader));
	// Buffer ready, push to destination pool
	dest_buffer_pool->push(dest_buffer);
	// We're now outsie the path of direct IO (maybe), so get a new buffer here so we can be ready for when we're in the direct IO
	// path
	_getDestBuffer();
}

/*
 * Packet decoder
 */

void PacketDecoder::_getDestBuffer() {
	// Grab a buffer to use for the resulting packet
	dest_buffer = avail_buffer_pool->pop();
	dest_buffer->clear();
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
	DEBUG_CERR("INCOMING PACKET SIZE: {}", size);

	// Make sure packet is big enough to contain our header
	if (size < sizeof(PacketHeader)) {
		CERR("Packet too small, should be > {}", sizeof(PacketHeader));
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
		return;
	}
	if (packet_header->reserved != 0) {
		CERR("Packet header should not have any reserved its set, it is {:02X}, DROPPING!",
			 static_cast<uint8_t>(packet_header->reserved));
		return;
	}
	// First thing we do is validate the header
	if (static_cast<uint8_t>(packet_header->format) &
		~(static_cast<uint8_t>(PacketHeaderFormat::ENCAPSULATED) | static_cast<uint8_t>(PacketHeaderFormat::COMPRESSED))) {
		CERR("Packet in invalid format {:02X}, DROPPING!", static_cast<uint8_t>(packet_header->format));
		return;
	}
	// Next check the channel is set to 0
	if (packet_header->channel) {
		CERR("Packet specifies invalid channel {:02X}, DROPPING!", static_cast<uint8_t>(packet_header->channel));
		return;
	}

	// Decode sequence
	uint32_t sequence = seth_be_to_cpu_32(packet_header->sequence);

	// If this is not the first packet...
	if (!first_packet) {
		// Check if we've lost packets
		if (sequence > last_sequence + 1) {
			CERR("PACKET LOST: last={}, seq={}, total_lost={}", last_sequence, sequence, sequence - last_sequence);
		}
		// If we we have an out of order one
		if (sequence < last_sequence + 1) {
			CERR("PACKET OOO : last={}, seq={}", last_sequence, sequence);
		}
	}

	// Next we check the headers that follow and read in any processing options
	// NK: we are not processing the packets here, just reading in options
	uint16_t opt_pos = sizeof(PacketHeader); // Our first option header starts after our packet header
	for (uint8_t header_num = 0; header_num < packet_header->opt_len; ++header_num) {
		// Make sure this option header can be read in the remaining data
		if (static_cast<ssize_t>(sizeof(PacketHeaderOption)) > size - opt_pos) {
			CERR("ERROR: Cannot read packet header option, not enough data");
			return;
		}
		// Overlay the option header
		PacketHeaderOption *packet_header_option = (PacketHeaderOption *)(packet + opt_pos);
		// Check for invalid header, where the reserved bits are set
		if (packet_header_option->reserved) {
			CERR("ERROR: Packet header option {} is invalid, reserved bits set", header_num);
			return;
		}
		// For now we just output the info
		CERR("  - Packet header option: header_num={}, type={:02X}", header_num, static_cast<uint8_t>(packet_header_option->type));
		// Bump the pos past the header option
		opt_pos += sizeof(PacketHeaderOption);
		// Bump the pos depending on the type of header option
		if (packet_header_option->type == PacketPayloadHeaderType::PARTIAL_PACKET) {
			opt_pos += sizeof(PacketHeaderOptionPartial);
		}
	}
	return; /*
	 opt_pos = sizeof(PacketHeader);

	 if (size < sizeof(PacketHeader)) {

	 }

	 // Next we overlay the packet payload header
	 // NK: we need to use a C-style cast here as reinterpret_cast casts against a single byte
	 PacketHeaderOption *packet_payload_header = (PacketHeaderOption *)packet + sizeof(PacketHeader);

	 if (packet_header->format == PacketHeaderFormat::ENCAPSULATED) {
		 dest_buffer->append(packet + sizeof(PacketHeader), size - sizeof(PacketHeader));
	 }

	 return;
	 // Buffer ready, push to destination pool
	 dest_buffer_pool->push(dest_buffer);
	 // We're now outsie the path of direct IO (maybe), so get a new buffer here so we can be ready for when we're in the direct
	 // IO path
	 _getDestBuffer(); */
}
