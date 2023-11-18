/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "Endian.hpp"
#include "PacketBuffer.hpp"
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
