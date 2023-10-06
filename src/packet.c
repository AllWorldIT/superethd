#include "packet.h"

#include <lz4.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffers.h"
#include "common.h"
#include "debug.h"

/**
 * @brief Detect if the packet sequence is wrapping.
 *
 * @param cur Current sequence.
 * @param prev Previous sequence.
 * @return int 1 if wrapping, 0 if not.
 */
int sequence_wrapping(uint32_t cur, uint32_t prev) {
	// Check if the current sequence number is less than the previous one
	// while accounting for wrapping
	return cur < prev && (prev - cur) < (UINT32_MAX / 2);
}

/**
 * @brief Encode packets for transmission over tunnel.
 *
 * Encode raw packet received from TAP into write buffers to be sent over the tunnel.
 *
 * @param wbuffers List of buffers used to write to the tunnel.
 * @param cbuffer Compression buffer, much larger than a normal buffer used for data compression.
 * @param pbuffer Packet buffer, this is the packet we're processing.
 * @param mss Maximum segment size of the data we're going to transmit.
 * @param seq Last sent sequence.
 * @return Number of packets ready in the wbuffers.
 */
int packet_encoder(BufferList *wbuffers, Buffer *cbuffer, Buffer *pbuffer, uint16_t mss, uint32_t seq) {
	// Packet header pointer
	PacketHeader *pkthdr;

	// This is the payload info where the final payload is copied from
	Buffer *payload;
	// Header changes
	uint8_t header_format = 0;
	// Try compress the packet and see if we can use it
	int csize = LZ4_compress_default(pbuffer->contents, cbuffer->contents, pbuffer->length, STAP_BUFFER_SIZE_COMPRESSION);
	if (csize < 1) {
		fprintf(stderr, "%s(): Failed to compress buffer\n", __func__);
		exit(EXIT_FAILURE);
	} else {
		// Set compression buffer length
		cbuffer->length = csize;
		// Now check if we actually saved space
		if (csize <= pbuffer->length - STAP_PACKET_HEADER_SIZE) {
			DEBUG_PRINT("Compression ratio: %.1f%% reduction (%i vs %lu)\n",
						((float)(pbuffer->length - csize) / pbuffer->length) * 100, csize, pbuffer->length);
			header_format |= STAP_PACKET_FORMAT_COMPRESSED;
			payload = cbuffer;
			// If the data was not compressed sufficiently, set the payload to the pbuffer contents
		} else {
			payload = pbuffer;
		}
	}

	// TODO: if payload > mss handle overzied payloads
	// TODO: remember payload header

	// Start at the beginning of wbuffers
	int wbuffer_count = 0;
	BufferNode *wbuffer_node = wbuffers->head;

	{
		Buffer *wbuffer = wbuffer_node->buffer;
		// Set up packet header, we point it to the buffer
		pkthdr = (PacketHeader *)wbuffer->contents;

		// Build packet header
		pkthdr->ver = STAP_PACKET_VERSION_V1;
		pkthdr->oam = 0;
		pkthdr->critical = 0;
		pkthdr->opt_len = 0;
		pkthdr->format = STAP_PACKET_FORMAT_ENCAP | header_format;
		pkthdr->channel = 1;
		pkthdr->sequence = stap_cpu_to_be_32(wbuffer_count + seq);
		// TODO: prefix packet payload with header
		// Copy packet in from the read buffer
		memcpy(wbuffer->contents + STAP_PACKET_HEADER_SIZE, payload->contents, payload->length);
		wbuffer->length = STAP_PACKET_HEADER_SIZE + payload->length;
		// Next buffer...
		wbuffer_node = wbuffer_node->next;
		wbuffer_count++;
	}

	return wbuffer_count;
}

/**
 * @brief Decode packets received over the tunnel.
 *
 * Decode raw packets received over the tunne by parsing the headers and dumping packets into wbuffers.
 *
 * @param wbuffers List of buffers used to write to TAP device.
 * @param cbuffer Compression buffer, much larger than a normal buffer used for data compression.
 * @param pbuffer Packet buffer, this is the packet we're processing.
 * @param mss Maximum segment size of the data we're going to transmit.
 * @param seq Last sent sequence.
 * @return Number of packets ready in the wbuffers.
 */
int packet_decoder(PacketDecoderState *state, BufferList *wbuffers, Buffer *cbuffer, Buffer *pbuffer, uint16_t mtu) {
	// Packet header pointer
	PacketHeader *pkthdr = (PacketHeader *)pbuffer->contents;

	// Grab sequence
	uint32_t seq = stap_be_to_cpu_32(pkthdr->sequence);

	// If this is not the first packet...
	if (!state->first_packet) {
		// Check if we've lost packets
		if (seq > state->last_seq + 1) {
			fprintf(stderr, "%s(): PACKET LOST: last=%u, seq=%u, total_lost=%u\n", __func__, state->last_seq, seq,
					seq - state->last_seq);
		}
		// If we we have an out of order one
		if (seq < state->last_seq + 1) {
			fprintf(stderr, "%s(): PACKET OOO : last=%u, seq=%u\n", __func__, state->last_seq, seq);
		}
	}

	// Start at the beginning of wbuffers
	int wbuffer_count = 0;
	BufferNode *wbuffer_node = wbuffers->head;
	// This will hold the pointer to the start of the actual payload containing packet(s)
	char *payload;
	int payload_length;

	// Depending on the format of the buffer we need to set up the payload pointer and size
	if (pkthdr->format & STAP_PACKET_FORMAT_COMPRESSED) {
		// Try decompress the contents
		int csize = LZ4_decompress_safe(pbuffer->contents + STAP_PACKET_HEADER_SIZE, cbuffer->contents,
										pbuffer->length - STAP_PACKET_HEADER_SIZE, STAP_BUFFER_SIZE_COMPRESSION);
		// And check if we ran into any errors
		if (csize < 1) {
			fprintf(stderr, "%s(): Failed to decompress packet!, DROPPING! error=%i\n", __func__, csize);
			goto bump_seq;
		}
        DEBUG_PRINT("Decompressed data size = %lu => %i\n", pbuffer->length - STAP_PACKET_HEADER_SIZE, csize);
		// If all is still good, set the payload to point to the decompressed data
		payload = cbuffer->contents;
		payload_length = csize;

	} else { /* if (pkthdr->format & STAP_PACKET_FORMAT_COMPRESSED) */
		// If the packet is not compressed, we just set the payload to point to the data following our packet header
		payload = pbuffer->contents + STAP_PACKET_HEADER_SIZE;
		payload_length = pbuffer->length - STAP_PACKET_HEADER_SIZE;
	}

	// Now we process the packets that come from the payload
	while (1) {
		// Make accessing wbuffer a bit easier
		Buffer *wbuffer = wbuffer_node->buffer;

		// TODO: FIND PACKETS HERE

		// Set packet location
		char *packet = payload;
		int packet_length = payload_length;
		// Do a few sanity checks on the packet
		if (packet_length > STAP_MAX_PACKET_SIZE) {
			fprintf(stderr, "%s(): DROPPING PAYLOAD! packet size %i exceeds maximum\n", __func__, packet_length);
			goto bump_seq;
		}
		if (packet_length > mtu + ETHERNET_FRAME_SIZE) {
			fprintf(stderr, "%s(): DROPPING PACKET! packet size %i bigger than MTU %i\n", __func__, packet_length, mtu);
			break;
		}

		// Copy payload into the wbuffer
		memcpy(wbuffer->contents, packet, packet_length);
		wbuffer->length = packet_length;

		// Next buffer...
		wbuffer_node = wbuffer_node->next;
		wbuffer_count++;

		break;
	}

bump_seq:
	// If this was the first packet, reset the first packet indicator
	if (state->first_packet) {
		state->first_packet = 0;
		state->last_seq = seq;
		// We only bump the last sequence if its smaller than the one we're on
	} else if (state->last_seq < seq) {
		state->last_seq = seq;
	} else if (state->last_seq > seq && sequence_wrapping(seq, state->last_seq)) {
		state->last_seq = seq;
	}

	return wbuffer_count;
}
