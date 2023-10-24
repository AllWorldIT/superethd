/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "buffers.hpp"
#include "Endian.hpp"

/*

   Packet Header:
	  |               |               |               |               |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |  Ver  |Opt Len|O|C|    Rsvd.  | Packet Format |    Channel    |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |                            Sequence                           |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |                                                               |
	  ~                    Variable-Length Options                    ~
	  |                                                               |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	- Version: 4 bits.
	- Opt Len: 4 bits, number of 32 bit wide variable options.
	- O: 1 bit, indicates a high priority management packet.
	- C: 1 bit, packet contains critical options that MUST be parsed.
	- Packet Format: 8 bits, this indicates the packet payload format.
	- Channel: 8 bits, Network channel to use.
*/

#define SETH_PACKET_VERSION_V1 0x1

#define SETH_PACKET_FORMAT_ENCAP 0x1
#define SETH_PACKET_FORMAT_COMPRESSED 0x2

#define SETH_PACKET_HEADER_SIZE 8

typedef struct __seth_packed {
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
	uint8_t format;
	uint8_t channel;
	seth_be32_t sequence;
	uint32_t opts[];
} PacketHeader;

/*
 *
 * Packet Payload Header:
 *    |               |               |               |               |
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *	  |     Type      |           Packet Size         |      RSVD     |
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *                      PARTIAL ADDITIONAL SECTION
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *	  |         Payload Length        |     PART      |      RSVD     |
 *	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

#define SETH_PACKET_PAYLOAD_TYPE_PARTIAL_PACKET 1
#define SETH_PACKET_PAYLOAD_TYPE_COMPLETE_PACKET 2

#define SETH_PACKET_PAYLOAD_HEADER_SIZE 4
#define SETH_PACKET_PAYLOAD_HEADER_PARTIAL_SIZE 4

typedef struct __seth_packed {
	uint8_t type;
	seth_be16_t packet_size;
	uint8_t reserved;
} PacketPayloadHeader;

typedef struct __seth_packed {
	seth_be16_t payload_length;
	uint8_t part;
	uint8_t reserved;
} PacketPayloadHeaderPartial;

// Packet encoder state
typedef struct {
	// Store the current packet sequence number
	uint32_t seq;

	// Write buffer info
	BufferNode *wbuffer_node;  // Current write buffer node
	uint16_t wbuffer_count;	   // Number of write buffers filled

	// Compression buffer
	Buffer cbuffer;

	// Payload and current pointer position for chunking
	Buffer *payload;
	uint16_t payload_pos;  // Position in payload we're currently at
	uint8_t payload_part;  // Payload sequence

} PacketEncoderState;


typedef struct {
	uint32_t seq; // Sequence number of the packet we're processing
	uint16_t decoded_packet_size; // Current payload header packet size

	int pos; // Encapsulating packet position
	int cur_header; // Counter of the header number we're processing in the encapsulating packet

	Buffer payload; // Pointer to the payload
} PacketDecoderEncapPacket;

// Packet decoder state
typedef struct {
	// Flag to indicate this is the first packet we're receiving
	int first_packet;
	// Last packet sequence number
	uint32_t last_seq;

	// Write buffer info
	BufferNode *wbuffer_node;  // Current write buffer node
	uint16_t wbuffer_count;	   // Number of write buffers filled

	// Compression buffer
	Buffer cbuffer;

	// Partial packet
	Buffer partial_packet;
	uint8_t partial_packet_part;	  // Payload sequence
	uint32_t partial_packet_lastseq;  // Last encap packet sequence
	uint16_t partial_packet_size;	  // The size of the partial packet we originally got
} PacketDecoderState;

size_t get_codec_wbuffer_count(uint16_t max_ethernet_frame_size);

void reset_packet_encoder(PacketEncoderState *state);
int init_packet_encoder(PacketEncoderState *state, uint16_t max_ethernet_frame_size);
int packet_encoder(PacketEncoderState *state, BufferList *wbuffers, Buffer *pbuffer, uint16_t mss);

int init_packet_decoder(PacketDecoderState *state, uint16_t max_packet_size);
int packet_decoder(PacketDecoderState *state, BufferList *wbuffers, Buffer *pbuffer, uint16_t mtu);
