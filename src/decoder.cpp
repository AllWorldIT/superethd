/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "decoder.hpp"
#include "codec.hpp"
#include "libaccl/logger.hpp"
#include "libaccl/stream_compressor_lz4.hpp"
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
	this->tx_buffer->clear();
	// Now for the packet parts we get, clear the info for them too
	this->last_part = 0;
	this->last_format = PacketHeaderOptionFormatType::NONE;
	this->last_orig_packet_size = 0;
}

/**
 * @brief Internal method to grab another TX buffer from the buffer pool.
 *
 */
void PacketDecoder::_getTxBuffer() {
	// Grab a buffer to use for the resulting packet
	this->tx_buffer = this->available_buffer_pool->pop_wait();
	this->_clearState();
}

/**
 * @brief Internal method to flush inflight buffers to the buffer pool.
 *
 */
void PacketDecoder::_flushInflight() {
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Flusing inflight buffers: pool=", this->available_buffer_pool->getBufferCount(),
					   ", count=", this->inflight_buffers.size());

	// Free buffers in flight
	if (!this->inflight_buffers.empty()) {
		this->available_buffer_pool->push(this->inflight_buffers);
	}
	// Clear decompressor state
	this->compressorLZ4->resetDecompressionStream();
	this->compressorZSTD->resetDecompressionStream();

	LOG_DEBUG_INTERNAL("  - INFLIGHT: Flusing inflight buffers after: pool=", this->available_buffer_pool->getBufferCount(),
					   ", count=", this->inflight_buffers.size());
}

/**
 * @brief Push a buffer into the inflight buffer list.
 *
 * @param packetBuffer
 */
void PacketDecoder::_pushInflight(std::unique_ptr<PacketBuffer> &packetBuffer) {
	// Push buffer into inflight list
	this->inflight_buffers.push_back(std::move(packetBuffer));
	LOG_DEBUG_INTERNAL("  - INFLIGHT: Packet added");
}

void PacketDecoder::_clearStateAndFlushInflight(std::unique_ptr<PacketBuffer> &packetBuffer) {
	// Clear current state
	this->_clearState();
	// Flush inflight buffers
	this->_pushInflight(packetBuffer);
	this->_flushInflight();
}

/**
 * @brief Construct a new packet decoder object
 *
 * @param l2mtu Layer 2 MTU size.
 * @param tx_buffer_pool TX buffer pool.
 * @param available_buffer_pool Available buffer pool.
 */
PacketDecoder::PacketDecoder(uint16_t l2mtu, std::shared_ptr<accl::BufferPool<PacketBuffer>> tx_buffer_pool,
							 std::shared_ptr<accl::BufferPool<PacketBuffer>> available_buffer_pool) {
	// As the constructor parameters have the same names as our data members, lets just use this-> for everything during init
	this->l2mtu = l2mtu;
	this->first_packet = true;

	// Initialize our compressors
	this->compressorLZ4 = new accl::StreamCompressorLZ4();
	this->compressorZSTD = new accl::StreamCompressorZSTD();

	// Grab a buffer to use for decompression
	dcomp_buffer = available_buffer_pool->pop_wait();

	// Setup buffer pools
	this->tx_buffer_pool = tx_buffer_pool;
	this->available_buffer_pool = available_buffer_pool;

	// Initialize the TX buffer for first use
	this->_getTxBuffer();
}

/**
 * @brief Destroy the Packet Decoder:: Packet Decoder object
 *
 */
PacketDecoder::~PacketDecoder() {
	delete this->compressorLZ4;
	delete this->compressorZSTD;
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
		this->available_buffer_pool->push(std::move(packetBuffer));
		this->tx_buffer->clear();
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
		this->_clearStateAndFlushInflight(packetBuffer);
		return;
	}
	// Make sure the reserved bits are not set
	if (packet_header->reserved != 0) {
		LOG_ERROR("Packet header should not have any reserved its set, it is ",
				  std::format("{:02X}", static_cast<unsigned int>(packet_header->reserved)), ", DROPPING!");
		// Clear current state and flush inflight buffers
		this->_clearStateAndFlushInflight(packetBuffer);
		return;
	}
	// We only support encapsulated packets at the moment
	if (static_cast<uint8_t>(packet_header->format) != static_cast<uint8_t>(PacketHeaderFormat::ENCAPSULATED)) {
		LOG_ERROR("Packet in invalid format ", std::format("{:02X}", static_cast<unsigned int>(packet_header->format)),
				  ", DROPPING!");
		// Clear current state and flush inflight buffers
		this->_clearStateAndFlushInflight(packetBuffer);
		return;
	}
	// Make sure the channel is not set
	if (packet_header->channel) {
		LOG_ERROR("Packet specifies invalid channel ", std::format("{:02X}", static_cast<unsigned int>(packet_header->channel)),
				  ", DROPPING!");
		// Clear current state and flush inflight buffers
		this->_clearStateAndFlushInflight(packetBuffer);
		return;
	}

	// Decode sequence
	uint32_t sequence = accl::be_to_cpu_32(packet_header->sequence);

	// If this is not the first packet...
	if (this->first_packet) {
		this->first_packet = false;
		this->last_sequence = sequence - 1;
	}

	// Check if we're in sequence if not we've lost packets
	if (sequence > this->last_sequence + 1) {
		LOG_INFO("{seq=", sequence, "}: Packet lost, last=", this->last_sequence, ", seq=", sequence,
				 ", total_lost=", sequence - this->last_sequence);
		// Clear current state and flush inflight buffers
		this->_clearState();
		this->_flushInflight();
	}
	// Check if we're out of order
	if (sequence < this->last_sequence + 1) {
		// Check that sequence hasn't wrapped
		if (is_sequence_wrapping(sequence, this->last_sequence)) {
			LOG_INFO("{seq=", sequence, "}: Sequence number probably wrapped: now=", sequence, ", prev=", this->last_sequence);
			this->last_sequence = 0;
		} else {
			LOG_NOTICE("{seq=", sequence, "}: PACKET OOO : last=", this->last_sequence, ", seq=", sequence);
			// Clear current state and flush inflight buffers
			this->_clearState();
			this->_flushInflight();
		}
	}

	// Save last sequence
	this->last_sequence = sequence;

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
			LOG_ERROR("{seq=", sequence, "}: Cannot read packet header option, buffer overrun");
			// Clear current state and flush inflight buffers
			this->_clearStateAndFlushInflight(packetBuffer);
			return;
		}
		// Overlay the option header
		PacketHeaderOption *packet_header_option = (PacketHeaderOption *)(packetBuffer->getData() + cur_pos);
		// Check for invalid header, where the reserved bits are set
		if (packet_header_option->reserved) {
			LOG_ERROR("{seq=", sequence, "}: Packet header option number ", static_cast<unsigned int>(header_num + 1),
					  " is invalid, reserved bits set");
			// Clear current state and flush inflight buffers
			this->_clearStateAndFlushInflight(packetBuffer);
			return;
		}
		// For now we just output the info
		LOG_DEBUG_INTERNAL("{seq=", sequence, "}:   - Packet header option: header=", static_cast<unsigned int>(header_num + 1),
						   ", type=", std::format("0x{:02X}", static_cast<unsigned int>(packet_header_option->type)));

		// If we don't have a packet_header_pos, we need to process options until we find one
		if (!packet_header_pos) {
			// Check if this is a valid packet header option type
			if (SETH_PACKET_HEADER_OPTION_TYPE_IS_VALID(packet_header_option)) {

				LOG_DEBUG_INTERNAL("{seq=", sequence, "}:   - Found packet header @", cur_pos);

				// Make sure our headers are in the correct positions, else something is wrong
				if (header_num + 1 != packet_header->opt_len) {
					LOG_ERROR(sequence,
							  ": Packet header should be the last header, current=", static_cast<unsigned int>(header_num + 1),
							  ", opt_len=", static_cast<unsigned int>(packet_header->opt_len));
					// Clear current state and flush inflight buffers
					this->_clearStateAndFlushInflight(packetBuffer);
					return;
				}

				// Set packet header position
				packet_header_pos = cur_pos;

				// If the type is unknown
			} else {
				// We don't know what this packet is, so lets just trash it
				LOG_ERROR("{seq=", sequence, "}: Packet header option number ", static_cast<unsigned int>(header_num + 1),
						  " has invalid type ", std::format("{:02X}", static_cast<unsigned int>(packet_header_option->type)));
				// Clear current state and flush inflight buffers
				this->_clearStateAndFlushInflight(packetBuffer);
				return;
			}
		}

		// Bump the pos past the header option
		cur_pos += sizeof(PacketHeaderOption);
	}

	/*
	 * Process packet
	 */

	// If we don't have a packet header, something is wrong, at present we should always have one
	if (!packet_header_pos) {
		LOG_ERROR("{seq=", sequence, "}: No packet header found, opt_len=", static_cast<unsigned int>(packet_header->opt_len),
				  "size=", packetBuffer->getDataSize());
		UT_ASSERT(packet_header_pos);
		// Clear current state and flush inflight buffers
		this->_clearStateAndFlushInflight(packetBuffer);
		return;
	}

	// This flag indicates if we're going to be flushing the inflight buffers
	bool flush_inflight = true;

	// Loop while the packet_header_pos is not the end of the encapsulated packet
	while (packet_header_pos < packetBuffer->getDataSize()) {

		// Packet header option
		PacketHeaderOption *packet_header_option = (PacketHeaderOption *)(packetBuffer->getData() + packet_header_pos);

		uint16_t orig_packet_size = accl::be_to_cpu_16(packet_header_option->packet_size);
		uint16_t payload_length = accl::be_to_cpu_16(packet_header_option->payload_length);

		// Make sure final packet size does not exceed the l2mtu
		if (orig_packet_size > this->l2mtu) {
			LOG_ERROR("{seq=", sequence, "}: Packet too big for interface L2MTU, ", orig_packet_size, " > ", this->l2mtu);
			// Clear current state and flush inflight buffers
			this->_clearStateAndFlushInflight(packetBuffer);
			return;
		}

		// When a packet is complete in a encapsulated packet
		if (packet_header_option->type == PacketHeaderOptionType::COMPLETE_PACKET) {
			uint16_t packet_pos = packet_header_pos + sizeof(PacketHeaderOption);

			LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - DECODE IS COMPLETE PACKET -");

			// Flush inflight if this is a complete packet
			flush_inflight = true;

			// Check if we had a last part?
			if (this->last_part) {
				LOG_NOTICE("{seq=", sequence, "}: At some stage we had a last part, but something got lost? clearing state");
				// Clear current state and flush inflight
				this->_clearState();
				this->_flushInflight();
			}

			// Packets cannot have a part set and be complete
			if (packet_header_option->part != 0) {
				LOG_ERROR("{seq=", sequence, "}: Packet part ", static_cast<unsigned int>(packet_header_option->part),
						  " is invalid, should be 0 for complete packet");
				// Clear current state and flush inflight buffers
				this->_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Make sure that our packet doesnt exceed the encapsulating packet size
			if (packet_pos + payload_length > packetBuffer->getDataSize()) {
				LOG_ERROR("{seq=", sequence, "}: Payload length ", payload_length, " would exceed encapsulating packet size ",
						  packetBuffer->getDataSize(), " at offset ", packet_pos, "by ",
						  packet_pos + payload_length - packetBuffer->getDataSize());
				// Clear current state and flush inflight buffers
				this->_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Next we check if its compressed
			if (SETH_PACKET_HEADER_OPTION_FORMAT_IS_COMPRESSED(packet_header_option)) {

				LOG_DEBUG_INTERNAL("{seq=", sequence, "}: Decompressing complete packet into tx_buffer, format ",
								   static_cast<unsigned int>(packet_header_option->format), " packet part ",
								   packet_header_option->part, "from pos ", packet_pos, " with size ", payload_length);

				// Try decompress buffer, we can decompress directly into the tx_buffer here as its a complete packet
				int decompressed_size;
				if (packet_header_option->format == PacketHeaderOptionFormatType::COMPRESSED_LZ4)
					decompressed_size =
						this->compressorLZ4->decompress(packetBuffer->getData() + packet_pos, payload_length,
														this->tx_buffer->getData(), this->tx_buffer->getBufferSize());
				else if (packet_header_option->format == PacketHeaderOptionFormatType::COMPRESSED_ZSTD) {
					decompressed_size =
						this->compressorZSTD->decompress(packetBuffer->getData() + packet_pos, payload_length,
														 this->tx_buffer->getData(), this->tx_buffer->getBufferSize());
				} else {
					LOG_ERROR("{seq=", sequence, "}: Packet has invalid format ",
							  std::format("{:02X}", static_cast<unsigned int>(packet_header_option->format)));
					// Clear current state and flush inflight buffers
					this->_clearStateAndFlushInflight(packetBuffer);
					return;
				}

				// If the result is <0 it would indicate some kind of an error happened during decompression
				if (decompressed_size < 0) {
					LOG_ERROR("{seq=", sequence, "}: Failed to decompress packet, error ", decompressed_size);
					// Clear current state and flush inflight buffers
					this->_clearStateAndFlushInflight(packetBuffer);
					return;
				}

				LOG_DEBUG_INTERNAL("{seq=", sequence, "}: Decompressed size ", decompressed_size,
								   " (orig_packet_size=", orig_packet_size, ")");

				// Set decompressed buffer size
				this->tx_buffer->setDataSize(decompressed_size);

				// Packet is not compressed, so we need to copy it out of the packet buffer into our tx_buffer
			} else {
				LOG_DEBUG_INTERNAL("{seq=", sequence, "}: Copy complete uncompressed packet from pos ", packet_pos, " with size ",
								   payload_length, " into tx_buffer at position ", this->tx_buffer->getDataSize());

				// Copy data into our buffer from the packet
				this->tx_buffer->append(packetBuffer->getData() + packet_pos, payload_length);
			}

			// Make sure the tx_buffer size size matches the original packet size
			if (this->tx_buffer->getDataSize() != orig_packet_size) {
				LOG_ERROR("{seq=", sequence,
						  "}: This should never happen, our packet tx_buffer_size=", this->tx_buffer->getDataSize(),
						  " does not match the packet size of ", orig_packet_size, ", DROPPING!!!");
				// Clear current state and flush inflight buffers
				this->_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Buffer ready, push to tx pool
			this->tx_buffer->setPacketSource(packetBuffer->getPacketSource());
			this->tx_buffer_pool->push(std::move(this->tx_buffer));

			// We're now outsie the path of direct IO, so get a new buffer here so we can be ready for when we're in the
			// direct IO path
			this->_getTxBuffer();

			// Bump the next packet header pos by the packet_pos plus the final_packet_size
			packet_header_pos = packet_pos + payload_length;

			// Handle partial packets
		} else if (SETH_PACKET_HEADER_OPTION_TYPE_IS_PARTIAL(packet_header_option)) {
			uint16_t packet_pos = packet_header_pos + sizeof(PacketHeaderOption);

			LOG_DEBUG_INTERNAL("{seq=", sequence, "}:  - DECODE IS PARIAL PACKET -");

			// Do not flush inflight if this is a partial packet
			flush_inflight = false;
			// Are we going to skip this packet? we do this if we lose something along the way and cannot rebuild it
			bool skip_packet = false;

			// If this is the first part and we had a previous part it probably means we lost something
			if (packet_header_option->part == 1 && this->last_part) {
				LOG_NOTICE("{seq=", sequence,
						   "}: Something got lost, header_part=", static_cast<unsigned int>(packet_header_option->part),
						   ", last_part=", static_cast<unsigned int>(this->last_part));
				// Clear current state and flush inflight
				this->_clearState();
				this->_flushInflight();
				skip_packet = true;

				// Verify part number is in sequence
			} else if (packet_header_option->part != this->last_part + 1) {
				LOG_NOTICE("{seq=", sequence, "}: Partial payload part ", static_cast<unsigned int>(packet_header_option->part),
						   " does not match last_part=", static_cast<unsigned int>(this->last_part), " + 1, SKIPPING!");
				// Clear current state and flush inflight
				this->_clearState();
				this->_flushInflight();
				// Skip this packet
				skip_packet = true;

				// Verify final packet size matches the last part, this should not change between parts
			} else if (this->last_part && orig_packet_size != this->last_orig_packet_size) {
				LOG_NOTICE("{seq=", sequence, "}: This final_packet_size=", orig_packet_size,
						   " does not match last_orig_packet_size=", this->last_orig_packet_size, ", SKIPPING!");
				// Clear current state and flush inflight
				this->_clearState();
				this->_flushInflight();
				// Skip this packet
				skip_packet = true;

				// Make sure packet format matches
			} else if (this->last_part && this->last_format != packet_header_option->format) {
				LOG_NOTICE("{seq=", sequence, "}: This packet format=", static_cast<unsigned int>(packet_header_option->format),
						   " does not match last_format=", static_cast<unsigned int>(this->last_format), ", SKIPPING!");
				// Clear current state and flush inflight
				this->_clearState();
				this->_flushInflight();
				// Skip this packet
				skip_packet = true;
			}

			// If we're skipping this packet, jump to bumping the header pos
			if (skip_packet) {
				LOG_DEBUG("{seq=", sequence, "}: Skipping unusable partial packet");
				goto bump_header_pos;
			}

			// Make sure that our encapsulated packet doesnt exceed the packet size
			// This should always be the case, even if its compressed
			if (packet_pos + payload_length > packetBuffer->getDataSize()) {
				LOG_ERROR("{seq=", sequence, "}: Encapsulated partial packet payload length ", payload_length,
						  " would exceed encapsulating packet size ", packetBuffer->getDataSize(), " at offset ", packet_pos);
				// Clear current state and flush inflight buffers
				this->_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Make sure the payload will fit into the TX buffer
			// This should also be the case even if its compressed
			if (this->tx_buffer->getDataSize() + payload_length > this->tx_buffer->getBufferSize()) {
				LOG_ERROR("{seq=", sequence, "}: Partial payload length of ", payload_length, " plus current buffer data size of ",
						  this->tx_buffer->getDataSize(), " would exceed buffer size ", this->tx_buffer->getBufferSize());
				// Clear current state and flush inflight buffers
				this->_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// If the buffer is not compressed it should never exceed the original packet size
			if (!SETH_PACKET_HEADER_OPTION_FORMAT_IS_COMPRESSED(packet_header_option) &&
				this->tx_buffer->getDataSize() + payload_length > orig_packet_size) {
				LOG_ERROR("{seq=", sequence,
						  "}: This should never happen, our packet tx_buffer_size=", this->tx_buffer->getDataSize(),
						  " + payload_length=", payload_length, " is bigger than the packet buffer of ",
						  packetBuffer->getDataSize(), ", DROPPING!!!");
				// Clear current state and flush inflight buffers
				this->_clearStateAndFlushInflight(packetBuffer);
				return;
			}

			// Append packet to TX buffer
			LOG_DEBUG_INTERNAL("{seq=", sequence, "}: Copy packet from pos ", packet_pos, " with size ", payload_length,
							   " into tx_buffer at position ", this->tx_buffer->getDataSize());
			this->tx_buffer->append(packetBuffer->getData() + packet_pos, payload_length);

			// Check if the packet is marked as completed
			if (SETH_PACKET_HEADER_OPTION_TYPE_IS_COMPLETE(packet_header_option)) {

				// Next we check if its compressed
				if (SETH_PACKET_HEADER_OPTION_FORMAT_IS_COMPRESSED(packet_header_option)) {

					LOG_DEBUG_INTERNAL("{seq=", sequence, "}: Partial packet complete and compressed, decompressing format ",
									   static_cast<unsigned int>(packet_header_option->format), ", size ",
									   this->tx_buffer->getDataSize());

					// Try decompress buffer
					int decompressed_size;
					if (packet_header_option->format == PacketHeaderOptionFormatType::COMPRESSED_LZ4)
						decompressed_size =
							this->compressorLZ4->decompress(this->tx_buffer->getData(), this->tx_buffer->getDataSize(),
															dcomp_buffer->getData(), dcomp_buffer->getBufferSize());
					else if (packet_header_option->format == PacketHeaderOptionFormatType::COMPRESSED_ZSTD) {
						decompressed_size = compressorZSTD->decompress(this->tx_buffer->getData(), this->tx_buffer->getDataSize(),
																	   dcomp_buffer->getData(), dcomp_buffer->getBufferSize());
					} else {
						LOG_ERROR("{seq=", sequence, "}: Packet has invalid format ",
								  std::format("{:02X}", static_cast<unsigned int>(packet_header_option->format)));
						// Clear current state and flush inflight buffers
						this->_clearStateAndFlushInflight(packetBuffer);
						return;
					}

					// If the result is <0 it would indicate some kind of an error happened during decompression
					if (decompressed_size < 0) {
						LOG_ERROR("{seq=", sequence, "}: Failed to decompress packet, error ", decompressed_size);
						// Clear current state and flush inflight buffers
						this->_clearStateAndFlushInflight(packetBuffer);
						return;
					}

					LOG_DEBUG_INTERNAL("{seq=", sequence, "}: Decompressed size ", decompressed_size,
									   " (orig_packet_size=", orig_packet_size, ")");

					// Set decompressed buffer size
					dcomp_buffer->setDataSize(decompressed_size);

					// Swap buffers
					std::swap(dcomp_buffer, this->tx_buffer);
				}

				// Check the tx_buffer size matches the original packet size
				if (this->tx_buffer->getDataSize() != orig_packet_size) {
					LOG_ERROR("{seq=", sequence,
							  "}: This should never happen, our packet tx_buffer_size=", this->tx_buffer->getDataSize(),
							  " does not match the packet size of ", orig_packet_size, ", DROPPING!!!");
					// Clear current state and flush inflight buffers
					this->_clearStateAndFlushInflight(packetBuffer);
					return;
				}

				LOG_DEBUG_INTERNAL("{seq=", sequence,
								   "}:   - Entire packet read... dumping into tx_buffer_pool & flushing inflight");
				// Buffer ready, push to TX pool
				this->tx_buffer->setPacketSource(packetBuffer->getPacketSource());
				this->tx_buffer_pool->push(std::move(this->tx_buffer));
				// We're now outsie the path of direct IO (maybe), so get a new buffer here so we can be ready for when we're in
				// the direct IO path
				this->_getTxBuffer();
				// We always flush inflight buffers when we have assembled a partial packet
				this->_flushInflight();

				goto bump_header_pos;
			}

			// This is not marked as a complete packet, so lets continue
			LOG_DEBUG_INTERNAL("{seq=", sequence, "}:   - Packet not entirely read, we need more");
			// Bump last part and set last_final_packet_size
			this->last_part = packet_header_option->part;
			this->last_format = packet_header_option->format;
			this->last_orig_packet_size = orig_packet_size;

		bump_header_pos:
			// Bump the next packet header pos by the packet_pos plus the packet payload length
			packet_header_pos = packet_pos + payload_length;
		}
	}

	// Add packet to inflight list so its flushed to the available pool
	this->_pushInflight(packetBuffer);
	// Check if we're flushing the inflight buffers
	if (flush_inflight) {
		this->_flushInflight();
	}
}
