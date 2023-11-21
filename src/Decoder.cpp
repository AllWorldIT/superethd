/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "Decoder.hpp"
#include "Codec.hpp"
#include "libaccl/Logger.hpp"
#include "libaccl/StreamCompressorLZ4.hpp"
#include "util.hpp"
#include <cassert>

/*
 * Packet decoder
 */

/**
 * @brief Internal method to clear the current decoder state.
 *
 */
void PacketDecoder::_clearState() {
	// Clear TX buffer
	tx_buffer->clear();
	// Now for the packet parts we get, clear the info for them too
	last_part = 0;
	last_orig_packet_size = 0;
}

/**
 * @brief Internal method to grab another TX buffer from the buffer pool.
 *
 */
void PacketDecoder::_getTxBuffer() {
	// Grab a buffer to use for the resulting packet
	tx_buffer = buffer_pool->pop_wait();
	_clearState();
}

/**
 * @brief Internal method to flush inflight buffers to the buffer pool.
 *
 */
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

/**
 * @brief Push a buffer into the inflight buffer list.
 *
 * @param packetBuffer
 */
void PacketDecoder::_pushInflight(std::unique_ptr<PacketBuffer> &packetBuffer) {
	// Push buffer into inflight list
	inflight_buffers.push_back(std::move(packetBuffer));
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Packet added");
}

void PacketDecoder::_clearStateAndFlushInflight(std::unique_ptr<PacketBuffer> &packetBuffer) {
	// Clear current state
	_clearState();
	// Clear decompressor state
	compressorLZ4->resetDecompressionStream();
	compressorBlosc2->resetDecompressionStream();
	// Flush inflight buffers
	_pushInflight(packetBuffer);
	_flushInflight();
}

/**
 * @brief Construct a new packet decoder object
 *
 * @param l2mtu Layer 2 MTU size.
 * @param available_buffer_pool Available buffer pool.
 * @param tx_buffer_pool TX buffer pool.
 */
PacketDecoder::PacketDecoder(uint16_t l2mtu, accl::BufferPool<PacketBuffer> *available_buffer_pool,
							 accl::BufferPool<PacketBuffer> *tx_buffer_pool) {
	// As the constructor parameters have the same names as our data members, lets just use this-> for everything during init
	this->l2mtu = l2mtu;
	this->first_packet = true;

	// Initialize our compressor
	this->compressorLZ4 = new accl::StreamCompressorLZ4();
	this->compressorBlosc2 = new accl::StreamCompressorBlosc2();

	// Grab a buffer to use for the compressor
	auto compressed_packet_buffer = available_buffer_pool->pop_wait();

	// Setup buffer pools
	this->buffer_pool = available_buffer_pool;
	this->tx_buffer_pool = tx_buffer_pool;

	// Initialize the TX buffer for first use
	_getTxBuffer();
}

/**
 * @brief Destroy the Packet Decoder:: Packet Decoder object
 *
 */
PacketDecoder::~PacketDecoder() {
	delete compressorLZ4;
	delete compressorBlosc2;
};

/**
 * @brief Decode buffer.
 *
 * @param packetBuffer Packet buffer.
 */
void PacketDecoder::decode(std::unique_ptr<PacketBuffer> packetBuffer) {
	LOG_DEBUG_INTERNAL("DECODER INCOMING PACKET SIZE: ", packetBuffer->getDataSize());

	// Make sure packet is big enough to contain our header
	if (packetBuffer->getDataSize() < sizeof(PacketHeader)) {
		LOG_ERROR("Packet too small, should be > ", sizeof(PacketHeader));
		buffer_pool->push(std::move(packetBuffer));
		tx_buffer->clear();
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
	if (static_cast<uint8_t>(packet_header->format) != static_cast<uint8_t>(PacketHeaderFormat::ENCAPSULATED)) {
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

	// Save last sequence
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
		LOG_DEBUG_INTERNAL("{seq=", sequence, "}:   - Packet header option: header=", static_cast<unsigned int>(header_num + 1),
						   ", type=", std::format("0x{:02X}", static_cast<unsigned int>(packet_header_option->type)));

		// If we don't have a packet_packet_header_pos, we need to process options until we find one
		if (!packet_header_pos) {
			// Check if this is a packet header
			if (static_cast<uint8_t>(packet_header_option->type) == static_cast<uint8_t>(PacketHeaderOptionType::COMPLETE_PACKET) ||
				static_cast<uint8_t>(packet_header_option->type) == static_cast<uint8_t>(PacketHeaderOptionType::PARTIAL_PACKET)) {

				LOG_DEBUG_INTERNAL("{seq=", sequence, "}:   - Found packet header @", cur_pos);

				// Make sure our headers are in the correct positions, else something is wrong
				if (header_num + 1 != packet_header->opt_len) {
					LOG_ERROR(sequence,
							  ": Packet header should be the last header, current=", static_cast<unsigned int>(header_num + 1),
							  ", opt_len=", static_cast<unsigned int>(packet_header->opt_len));
					// Clear current state and flush inflight buffers
					_clearStateAndFlushInflight(packetBuffer);
					return;
				}

				// Set packet header position
				packet_header_pos = cur_pos;

				// If the type is unknown
			} else {
				// We don't know what this packet is, so lets just trash it
				LOG_ERROR(sequence, ": Packet header option number ", static_cast<unsigned int>(header_num + 1),
						  " has invalid type ", std::format("{:02X}", static_cast<unsigned int>(packet_header_option->type)));
				// Clear current state and flush inflight buffers
				_clearStateAndFlushInflight(packetBuffer);
				return;
			}
		}

		// Bump the pos past the header option
		cur_pos += sizeof(PacketHeaderOption);
	}

	/*
	 * Process packet
	 */

	// If we don't have a packet header, something is wrong
	if (!packet_header_pos) {
		LOG_ERROR(sequence, ": No packet header found, opt_len=", static_cast<unsigned int>(packet_header->opt_len),
				  "size=", packetBuffer->getDataSize());
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

		uint16_t orig_packet_size = seth_be_to_cpu_16(packet_header_option->packet_size);
		uint16_t payload_length = seth_be_to_cpu_16(packet_header_option->payload_length);

		// Make sure final packet size does not exceed MTU + ETHERNET_HEADER_LEN
		if (orig_packet_size > l2mtu) {
			LOG_ERROR(sequence, ": Packet too big for interface L2MTU, ", orig_packet_size, " > ", l2mtu);
			// Clear current state and flush inflight buffers
			_clearStateAndFlushInflight(packetBuffer);
			return;
		}

		// When a packet is completely inside the encap packet
		if (packet_header_option->type == PacketHeaderOptionType::COMPLETE_PACKET) {
			uint16_t packet_pos = packet_header_pos + sizeof(PacketHeaderOption);

			LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - DECODE IS COMPLETE PACKET -");

			// Flush inflight if this is a complete packet
			flush_inflight = true;

			// Check if we had a last part?
			if (last_part) {
				LOG_NOTICE(sequence, ": At some stage we had a last part, but something got lost? clearing state");
				// Clear current state and flush inflight
				_clearState();
				_flushInflight();
			}

			// Make sure the packet part is 0, this is a complete packet, it does not have parts
			if (packet_header_option->part != 0) {
				LOG_ERROR(sequence, ": Packet part ", static_cast<unsigned int>(packet_header_option->part),
						  " is invalid, should be 0 for complete packet");
				// Clear current state and flush inflight buffers
				_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Make sure that our encapsulated packet doesnt exceed the encapsulating packet size
			if (packet_pos + payload_length > packetBuffer->getDataSize()) {
				LOG_ERROR(sequence, ": Payload length ", payload_length, " would exceed encapsulating packet size ",
						  packetBuffer->getDataSize(), " at offset ", packet_pos, "by ",
						  packet_pos + payload_length - packetBuffer->getDataSize());
				// Clear current state and flush inflight buffers
				_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Check what format this packet is in
			if (packet_header_option->format == PacketHeaderOptionFormatType::NONE) {
				LOG_DEBUG_INTERNAL("{seq=", sequence, "}: Copy packet from pos ", packet_pos, " with size ", orig_packet_size,
								   " into tx_buffer at position ", tx_buffer->getDataSize());

				// First we handle plain packets
				tx_buffer->append(packetBuffer->getData() + packet_pos, payload_length);

				// Next we're going to look for compressed packets
			} else if (SETH_PACKET_HEADER_OPTION_FORMAT_IS_VALID(packet_header_option)) {

				// Try decompress buffer
				int decompressed_size;
				if (packet_header_option->format == PacketHeaderOptionFormatType::COMPRESSED_LZ4)
					decompressed_size = compressorLZ4->decompress(packetBuffer->getData() + packet_pos, payload_length,
																  tx_buffer->getData(), tx_buffer->getBufferSize());

				else if (packet_header_option->format == PacketHeaderOptionFormatType::COMPRESSED_BLOSC2) {
					decompressed_size = compressorBlosc2->decompress(packetBuffer->getData() + packet_pos, payload_length,
																	 tx_buffer->getData(), tx_buffer->getBufferSize());
				} else {
					LOG_ERROR(sequence, ": Packet has invalid format ",
							  std::format("{:02X}", static_cast<unsigned int>(packet_header_option->format)));
					// Clear current state and flush inflight buffers
					_clearStateAndFlushInflight(packetBuffer);
					return;
				}

				// If the result is <0 it would indicate some kind of an error happened during decompression
				if (decompressed_size < 0) {
					LOG_ERROR(sequence, ": Failed to decompress packet, error ", decompressed_size);
					// Clear current state and flush inflight buffers
					_clearStateAndFlushInflight(packetBuffer);
					return;
				}
				LOG_DEBUG_INTERNAL("{seq=", sequence, "}: Decompressed packet from pos ", packet_pos, " with size ", payload_length,
								   " into tx_buffer at position ", tx_buffer->getDataSize(), " with decompressed size ",
								   decompressed_size, " (orig_packet_size=", orig_packet_size, ")");
				tx_buffer->setDataSize(decompressed_size);

				// Lastly we have a fallthrough if the format is unknown
			} else {
				LOG_ERROR(sequence, ": Packet has invalid format ",
						  std::format("{:02X}", static_cast<unsigned int>(packet_header_option->format)));
				// Clear current state and flush inflight buffers
				_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Check that tx_buffer now matches the original packet size
			if (tx_buffer->getDataSize() != orig_packet_size) {
				LOG_ERROR(sequence, ": This should never happen, our packet tx_buffer_size=", tx_buffer->getDataSize(),
						  " does not match the packet size of ", orig_packet_size, ", DROPPING!!!");
				// Clear current state and flush inflight buffers
				_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Buffer ready, push to tx pool
			tx_buffer_pool->push(std::move(tx_buffer));

			// We're now outsie the path of direct IO, so get a new buffer here so we can be ready for when we're in the
			// direct IO path
			_getTxBuffer();

			// Bump the next packet header pos by the packet_pos plus the final_packet_size
			packet_header_pos = packet_pos + payload_length;

			// when a packet is split between encap packets
		} else if (packet_header_option->type == PacketHeaderOptionType::PARTIAL_PACKET) {
			uint16_t packet_pos = packet_header_pos + sizeof(PacketHeaderOption);

			LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - DECODE IS PARIAL PACKET -");

			// Do not flush inflight if this is a partial packet
			flush_inflight = false;
			// Are we going to skip this packet? we do this if we lose something along the way and cannot rebuild it
			bool skip_packet = false;

			// If our part is 0, it means we probably lost something if we had a last_part, we can clear the buffer and reset
			// last_part
			if (packet_header_option->part == 1 && last_part) {
				LOG_NOTICE(sequence, ": Something got lost, header_part=", static_cast<unsigned int>(packet_header_option->part),
						   ", last_part=", static_cast<unsigned int>(last_part));
				// Clear current state and flush inflight
				_clearState();
				_flushInflight();
				skip_packet = true;

				// Verify part number
			} else if (packet_header_option->part != last_part + 1) {
				LOG_NOTICE(sequence, ": Partial payload part ", static_cast<unsigned int>(packet_header_option->part),
						   " does not match last_part=", static_cast<unsigned int>(last_part), " + 1, SKIPPING!");
				// Clear current state and flush inflight
				_clearState();
				_flushInflight();
				// Skip this packet
				skip_packet = true;

				// Verify packet size is the same as the last one
			} else if (last_part && orig_packet_size != last_orig_packet_size) {
				LOG_NOTICE(sequence, ": This final_packet_size=", orig_packet_size,
						   " does not match last_orig_packet_size=", last_orig_packet_size, ", SKIPPING!");
				// Clear current state and flush inflight
				_clearState();
				_flushInflight();
				// Skip this packet
				skip_packet = true;
			}

			// If we're skipping this packet, jump to the bumping the header pos
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

			// Check what format this packet is in
			if (packet_header_option->format == PacketHeaderOptionFormatType::NONE) {
				// Make sure the payload will fit into the TX buffer
				if (tx_buffer->getDataSize() + payload_length > tx_buffer->getBufferSize()) {
					LOG_ERROR(sequence, ": Partial payload length of ", payload_length, " plus current buffer data size of ",
							  tx_buffer->getDataSize(), " would exceed buffer size ", tx_buffer->getBufferSize());
					// Clear current state and flush inflight buffers
					_clearStateAndFlushInflight(packetBuffer);
					return;
				}
				// Check if the current buffer plus the payload we got now exceeds what the final packet size should be
				if (tx_buffer->getDataSize() + payload_length > orig_packet_size) {
					LOG_ERROR(sequence, ": This should never happen, our packet tx_buffer_size=", tx_buffer->getDataSize(),
							  " + payload_length=", payload_length, " is bigger than the packet buffer of ",
							  packetBuffer->getDataSize(), ", DROPPING!!!");
					// Clear current state and flush inflight buffers
					_clearStateAndFlushInflight(packetBuffer);
					return;
				}

				// Append packet to TX buffer
				LOG_DEBUG_INTERNAL("{seq=", sequence, "}: Copy packet from pos ", packet_pos, " with size ", payload_length,
								   " into tx_buffer at position ", tx_buffer->getDataSize());
				tx_buffer->append(packetBuffer->getData() + packet_pos, payload_length);

				// Next we're going to look for LZ4 compressed packets
			} else if (SETH_PACKET_HEADER_OPTION_FORMAT_IS_VALID(packet_header_option)) {
				LOG_DEBUG_INTERNAL("{seq=", sequence, "}: Decompressing format ",
								   static_cast<unsigned int>(packet_header_option->format), " packet part ",
								   packet_header_option->part, "from pos ", packet_pos, " with size ", orig_packet_size,
								   " into tx_buffer at position ", tx_buffer->getDataSize(), " with output size ",
								   tx_buffer->getBufferSize() - tx_buffer->getDataSize());

				// Try decompress buffer
				int decompressed_size;
				if (packet_header_option->format == PacketHeaderOptionFormatType::COMPRESSED_LZ4)
					decompressed_size = compressorLZ4->decompress(packetBuffer->getData() + packet_pos, payload_length,
																  tx_buffer->getData() + tx_buffer->getDataSize(),
																  tx_buffer->getBufferSize() - tx_buffer->getDataSize());
				else if (packet_header_option->format == PacketHeaderOptionFormatType::COMPRESSED_BLOSC2) {
					decompressed_size = compressorBlosc2->decompress(packetBuffer->getData() + packet_pos, payload_length,
																	 tx_buffer->getData() + tx_buffer->getDataSize(),
																	 tx_buffer->getBufferSize() - tx_buffer->getDataSize());
				} else {
					LOG_ERROR(sequence, ": Packet has invalid format ",
							  std::format("{:02X}", static_cast<unsigned int>(packet_header_option->format)));
					// Clear current state and flush inflight buffers
					_clearStateAndFlushInflight(packetBuffer);
					return;
				}

				// If the result is <0 it would indicate some kind of an error happened during decompression
				if (decompressed_size < 0) {
					LOG_ERROR(sequence, ": Failed to decompress packet, error ", decompressed_size);
					// Clear current state and flush inflight buffers
					_clearStateAndFlushInflight(packetBuffer);
					return;
				}
				LOG_DEBUG_INTERNAL(
					"{seq=", sequence, "}: Decompressed format ", static_cast<unsigned int>(packet_header_option->format),
					" packet part ", packet_header_option->part, "from pos ", packet_pos, " with size ", orig_packet_size,
					" into tx_buffer at position ", tx_buffer->getDataSize(), " with decompressed size ", decompressed_size);
				tx_buffer->setDataSize(tx_buffer->getDataSize() + decompressed_size);

				// Lastly we have a fallthrough if the format is unknown
			} else {
				LOG_ERROR(sequence, ": Packet has invalid format ",
						  std::format("{:02X}", static_cast<unsigned int>(packet_header_option->format)));
				// Clear current state and flush inflight buffers
				_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			if (tx_buffer->getDataSize() == orig_packet_size) {
				LOG_DEBUG_INTERNAL("{seq=", sequence,
								   "}:   - Entire packet read... dumping into tx_buffer_pool & flushing inflight");
				// Buffer ready, push to TX pool
				tx_buffer_pool->push(std::move(tx_buffer));
				// We're now outsie the path of direct IO (maybe), so get a new buffer here so we can be ready for when we're in
				// the direct IO path
				_getTxBuffer();
				// We always flush inflight buffers when we have assembled a partial packet
				_flushInflight();
			} else {
				LOG_DEBUG_INTERNAL("{seq=", sequence, "}:   - Packet not entirely read, we need more");
				// Bump last part and set last_final_packet_size
				last_part = packet_header_option->part;
				last_orig_packet_size = orig_packet_size;
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
