/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "Endian.hpp"
#include "common.hpp"
#include "libaccl/BufferPool.hpp"
#include "libsethnetkit/EthernetPacket.hpp"

// Maximum packet size
inline constexpr uint16_t SETH_PACKET_MAX_SIZE = UINT16_MAX - SETH_PACKET_ETHERNET_HEADER_LEN;

/*
 *
 * Packet Header:
 *	  |               |               |               |               |
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *	  |  Ver  |Opt Len|O|C|    Rsvd.  | Packet Format |    Channel    |
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *	  |                            Sequence                           |
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *	  |                                                               |
 *	  ~                    Variable-Length Options                    ~
 *	  |                                                               |
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *	- Version: 4 bits.
 *	- Opt Len: 4 bits, number of 32 bit wide variable options.
 *	- O: 1 bit, indicates a high priority management packet.
 *	- C: 1 bit, packet contains critical options that MUST be parsed.
 *	- Packet Format: 8 bits, this indicates the packet payload format.
 *	- Channel: 8 bits, Network channel to use.
 */

inline constexpr uint8_t SETH_PACKET_HEADER_VERSION_V1{0x1};

enum class PacketHeaderFormat : uint8_t {
	ENCAPSULATED = 0x1,
	COMPRESSED = 0x2,
};

struct PacketHeader {
#if SETH_BYTE_ORDER == SETH_BIG_ENDIAN
		uint8_t ver : 4;
		uint8_t opt_len : 4;

		uint8_t oam : 1;
		uint8_t critical : 1;
		uint8_t reserved : 6;
#else
		uint8_t opt_len : 4;
		uint8_t ver : 4;

		uint8_t reserved : 6;
		uint8_t critical : 1;
		uint8_t oam : 1;

#endif
		PacketHeaderFormat format;
		uint8_t channel; // Set to 0 if channels are not used
		seth_be32_t sequence;
} SETH_PACKED_ATTRIBUTES;

/*
 *
 * Packet Payload Header:
 *    |               |               |               |               |
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *	  |     Type      |           Packet Size         |      RSVD     |
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *                  HEADER OPTION - PARTIAL - DATA SECTION
 *    |               |               |               |               |
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *	  |         Payload Length        |     PART      |      RSVD     |
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

enum class PacketHeaderOptionType : uint8_t {
	PARTIAL_PACKET = 0x1,
	COMPLETE_PACKET = 0x2,
};

inline constexpr uint16_t SETH_PACKET_PAYLOAD_HEADER_SIZE{4};
inline constexpr uint16_t SETH_PACKET_PAYLOAD_HEADER_PARTIAL_SIZE{4};

struct PacketHeaderOption {
		PacketHeaderOptionType type;
		seth_be16_t packet_size;
		uint8_t reserved;
} SETH_PACKED_ATTRIBUTES;

struct PacketHeaderOptionPartialData {
		seth_be16_t payload_length;
		uint8_t part;
		uint8_t reserved;
} SETH_PACKED_ATTRIBUTES;

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
		// Active destination buffer that is not yet full
		std::unique_ptr<accl::Buffer> dest_buffer;
		// Buffers in flight, currently being utilized
		std::deque<std::unique_ptr<accl::Buffer>> inflight_buffers;
		// Active destination buffer header options length
		uint16_t opt_len;
		// Number of packets currently encoded
		uint32_t packet_count;

		// Buffer pool to get buffers from
		accl::BufferPool *buffer_pool;
		// Buffer pool to push buffers to
		accl::BufferPool *dest_buffer_pool;

		void _flush();

		void _getDestBuffer();

		uint16_t _getMaxPayloadSize(uint16_t size) const;

		void _flushInflight();
		void _pushInflight(std::unique_ptr<accl::Buffer> &packetBuffer);

	public:
		PacketEncoder(uint16_t l2mtu, uint16_t l4mtu, accl::BufferPool *available_buffer_pool,
					  accl::BufferPool *destination_buffer_pool);
		~PacketEncoder();

		void encode(std::unique_ptr<accl::Buffer> packetBuffer);

		void flush();
};

/*
 * Packet decoder
 */

class PacketDecoder {
	private:
		// Interface MTU
		uint16_t l2mtu;

		// Sequence counter
		bool first_packet;
		uint32_t last_sequence;
		uint8_t last_part;
		uint16_t last_final_packet_size;
		// Active destination buffer that is not yet full
		std::unique_ptr<accl::Buffer> dest_buffer;
		// Buffers in flight, currently being utilized
		std::deque<std::unique_ptr<accl::Buffer>> inflight_buffers;
		// Buffer pool to get buffers from
		accl::BufferPool *buffer_pool;
		// Buffer pool to push buffers to
		accl::BufferPool *dest_buffer_pool;

		void _clearState();

		void _getDestBuffer();

		void _flushInflight();
		void _pushInflight(std::unique_ptr<accl::Buffer> &packetBuffer);
		void _clearStateAndFlushInflight(std::unique_ptr<accl::Buffer> &packetBuffer);

	public:
		PacketDecoder(uint16_t l2mtu, accl::BufferPool *available_buffer_pool, accl::BufferPool *destination_buffer_pool);
		~PacketDecoder();

		void decode(std::unique_ptr<accl::Buffer> packetBuffer);
};
