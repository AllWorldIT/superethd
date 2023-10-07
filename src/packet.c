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
 * @brief Initialize the packet encoder.
 *
 * @param state Packet encoder state.
 * @return int 0 on success.
 */
int init_packet_encoder(PacketEncoderState *state) {
	// Packet sequence
	state->seq = 0;

	// Number of write buffers filled
	state->wbuffer_node = NULL;	 // Set to NULL once buffers are written
	state->wbuffer_count = 0;	 // Set to 0 on new encoding run

	// Allocate buffer used for compression
	state->cbuffer.contents = (char *)malloc(STAP_BUFFER_SIZE_COMPRESSION);
	state->cbuffer.length = 0;
	// Current payload we're dealing with
	state->payload = NULL;
	state->payload_pos = 0;	 // Set to 0 on new payload
	state->payload_seq = 0;	 // Set to 0 on new payload
	return 0;
}

/**
 * @brief Reset packet encoder state.
 *
 * @param state Packet encoder state.
 * @return int 0 on success.
 */
void reset_packet_encoder(PacketEncoderState *state) {
	state->wbuffer_node = NULL;	 // Set to NULL once buffers are written
}

/**
 * @brief Encode packets for transmission over tunnel.
 *
 * Encode raw packet received from TAP into write buffers to be sent over the tunnel.
 *
 * @param state Encoder state.
 * @param wbuffers List of buffers used to write to the tunnel.
 * @param pbuffer Packet buffer, this is the packet we're processing.
 * @param max_wbuffer_size Maximum size of the resulting packets generated, not including any encapsulating packet protocol headers.
 * @return Number of packets ready in the wbuffers.
 */
int packet_encoder(PacketEncoderState *state, BufferList *wbuffers, Buffer *pbuffer, uint16_t max_wbuffer_size) {
	// Packet header pointer
	PacketHeader *pkthdr;
	PacketPayloadHeaderComplete *pktphdr;

	// Header changes
	uint8_t header_format = 0;
	// Try compress the packet and see if we can use it
	int csize = LZ4_compress_default(pbuffer->contents, state->cbuffer.contents, pbuffer->length, STAP_BUFFER_SIZE_COMPRESSION);
	if (csize < 1) {
		FPRINTF("Failed to compress buffer");
		exit(EXIT_FAILURE);
	} else {
		// Set compression buffer length
		state->cbuffer.length = csize;
		// Now check if we actually saved space
		if (csize <= pbuffer->length - STAP_PACKET_HEADER_SIZE && 0) {	// FIXME: DISABLED FOR NOW
			DEBUG_PRINT("Compression ratio: %.1f%% reduction (%i vs %lu)",
						((float)(pbuffer->length - csize) / pbuffer->length) * 100, csize, pbuffer->length);
			header_format |= STAP_PACKET_FORMAT_COMPRESSED;
			state->payload = &state->cbuffer;
			// If the data was not compressed sufficiently, set the payload to the pbuffer contents
		} else {
			state->payload = pbuffer;
		}
		// Start with the new payload
		state->payload_pos = 0;
		state->payload_seq = 0;
	}

	// If we don't have a write buffer it means we flushed the previous one(s) and we need another
	if (!state->wbuffer_node) {
		state->wbuffer_node = wbuffers->head;
		state->wbuffer_node->buffer->length = 0;
		state->wbuffer_count = 0;
	}

	// We're going to fill the packet up at least until we have 128 bytes left at the end
	Buffer *wbuffer = state->wbuffer_node->buffer;
	while (state->payload_pos < state->payload->length) {
		int chunk_size = 0;
		int payload_header_type = 0;

		// Work out our header size, at the bare minimum we need the payload header
		int header_size = STAP_PACKET_PAYLOAD_HEADER_SIZE;
		// If our length is 0, it means its a new buffer, so we also nead the packet header
		if (!wbuffer->length) header_size += STAP_PACKET_HEADER_SIZE;
		DEBUG_PRINT("header_sizesize: %i", header_size);

		// Work out wbuffer space left
		int wbuffer_left = max_wbuffer_size - wbuffer->length - header_size;
		DEBUG_PRINT("wbuffer_left=%i  (mss=%i - wbuffer->length=%lu - header_size=%i)", wbuffer_left, max_wbuffer_size,
					wbuffer->length, header_size);

		// Work out payload data left
		int payload_left = state->payload->length - state->payload_pos;
		DEBUG_PRINT("payload_left=%i", payload_left);

		// Check if this is the first packet and if the entire thing will fit
		if (!state->payload_seq && payload_left < wbuffer_left) {  // Packet will fit
			chunk_size = state->payload->length;
			payload_header_type = STAP_PACKET_PAYLOAD_TYPE_COMPLETE_PACKET;
			DEBUG_PRINT("chunk_size=%i (payload_left=%i < wbuffer_left=%i) - PACKET FITS", chunk_size, payload_left, wbuffer_left);
		} else {  // Packet won't fit, so we try use the smallest of either the wbuffer_left or payload_left
			payload_header_type = STAP_PACKET_PAYLOAD_TYPE_PARTIAL_PACKET;
			// Bump the header size and reduce wbuffer_left, this header is bigger
			header_size += STAP_PACKET_PAYLOAD_HEADER_SIZE;
			wbuffer_left -= STAP_PACKET_PAYLOAD_HEADER_SIZE;
			chunk_size = (payload_left < wbuffer_left) ? payload_left : wbuffer_left;
			DEBUG_PRINT("chunk_size=%i (payload_left=%i, new wbuffer_left=%i, new header_size=%i)", chunk_size, payload_left,
						wbuffer_left, header_size);
		}

		// If the length is zero, it means we need a packet header
		if (!wbuffer->length) {
			// Set up packet header, we point it to the buffer
			pkthdr = (PacketHeader *)wbuffer->contents;
			// Build packet header
			pkthdr->ver = STAP_PACKET_VERSION_V1;
			pkthdr->oam = 0;
			pkthdr->critical = 0;
			pkthdr->opt_len = 1;
			pkthdr->format = STAP_PACKET_FORMAT_ENCAP | header_format;
			pkthdr->channel = 1;
			pkthdr->sequence = stap_cpu_to_be_32(state->seq);

			// Check if the counter is wrapping
			if (state->seq == UINT32_MAX)
				state->seq = 0;
			else
				state->seq++;

			// Bump the wbuffer length past the header, so we can write the packet payload header below
			wbuffer->length += STAP_PACKET_HEADER_SIZE;
		}

		// Write the packet payload header
		pktphdr = (PacketPayloadHeaderComplete *)&pkthdr->opts[0];
		pktphdr->type = payload_header_type;
		pktphdr->payload_size = stap_cpu_to_be_16(state->payload->length);
		pktphdr->reserved = 0;
		wbuffer->length += STAP_PACKET_PAYLOAD_HEADER_SIZE;
		// If this is a partial header, we overlay the partial header struct and set the additional values
		if (payload_header_type == STAP_PACKET_PAYLOAD_TYPE_PARTIAL_PACKET) {
			PacketPayloadHeaderPartial *pktphdr_partial = (PacketPayloadHeaderPartial *)&pkthdr->opts[1];
			pktphdr_partial->payload_length = stap_cpu_to_be_16(chunk_size);
			pktphdr_partial->seq = state->payload_seq;
			pktphdr_partial->reserved2 = 0;
			wbuffer->length += STAP_PACKET_PAYLOAD_HEADER_SIZE;
		}

		// Copy chunk into buffer
		memcpy(wbuffer->contents + wbuffer->length, state->payload->contents + state->payload_pos, chunk_size);
		// Bump all our states
		state->payload_pos += chunk_size;
		wbuffer->length += chunk_size;
		state->payload_seq++;
		DEBUG_PRINT("NEW: payload_pos=%lu, wbuffer->length=%lu, payload_seq=%i", state->payload_pos, wbuffer->length,
					state->payload_seq);

		// Check if we have finished our payload, or finished the buffer
		// TODO: this bumps the wbuffer, we don't really need to do it if the payload is done, we can wait for a timeout
		if (state->payload_pos == state->payload->length || wbuffer->length == max_wbuffer_size) {
			state->wbuffer_count++;
			state->wbuffer_node = state->wbuffer_node->next;
			wbuffer = state->wbuffer_node->buffer;
			wbuffer->length = 0;
		}
	}

	DEBUG_PRINT("DONE: wbuffer_count=%i", state->wbuffer_count);

	return state->wbuffer_count;
}

/**
 * @brief Initialize the packet decoder.
 *
 * @param state Packet decoder state.
 * @return int 0 on success.
 */
int init_packet_decoder(PacketDecoderState *state) {
	state->first_packet = 1;
	state->last_seq = 0;
	// Allocate buffer used for decompression and assembly
	state->cbuffer.contents = (char *)malloc(STAP_BUFFER_SIZE_COMPRESSION);
	state->cbuffer.length = 0;
	return 0;
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
int packet_decoder(PacketDecoderState *state, BufferList *wbuffers, Buffer *pbuffer, uint16_t mtu) {
	// Packet header pointer
	PacketHeader *pkthdr = (PacketHeader *)pbuffer->contents;
	PacketPayloadHeaderComplete *pktphdr;

	// Grab sequence
	uint32_t seq = stap_be_to_cpu_32(pkthdr->sequence);

	// If this is not the first packet...
	if (!state->first_packet) {
		// Check if we've lost packets
		if (seq > state->last_seq + 1) {
			FPRINTF("PACKET LOST: last=%u, seq=%u, total_lost=%u", state->last_seq, seq, seq - state->last_seq);
		}
		// If we we have an out of order one
		if (seq < state->last_seq + 1) {
			FPRINTF("PACKET OOO : last=%u, seq=%u", state->last_seq, seq);
		}
	}

	// Start at the beginning of wbuffers
	int wbuffer_count = 0;
	BufferNode *wbuffer_node = wbuffers->head;
	// This will hold the pointer to the start of the actual payload containing packet(s)
	Buffer payload;

	// Depending on the format of the buffer we need to set up the payload pointer and size
	if (pkthdr->format == (STAP_PACKET_FORMAT_ENCAP & STAP_PACKET_FORMAT_COMPRESSED)) {
		// Try decompress the contents
		state->cbuffer.length = LZ4_decompress_safe(pbuffer->contents + STAP_PACKET_HEADER_SIZE, state->cbuffer.contents,
													pbuffer->length - STAP_PACKET_HEADER_SIZE, STAP_BUFFER_SIZE_COMPRESSION);
		// And check if we ran into any errors
		if (state->cbuffer.length < 1) {
			FPRINTF("Failed to decompress packet!, DROPPING! error=%lu", state->cbuffer.length);
			goto bump_seq;
		}
		DEBUG_PRINT("Decompressed data size = %lu => %lu", pbuffer->length - STAP_PACKET_HEADER_SIZE, state->cbuffer.length);
		// If all is still good, set the payload to point to the decompressed data
		payload.contents = state->cbuffer.contents;
		payload.length = state->cbuffer.length;
		// Plain uncompressed encapsulated packets
	} else if (pkthdr->format == STAP_PACKET_FORMAT_ENCAP) {
		// If the packet is not compressed, we just set the payload to point to the data following our packet header
		payload.contents = pbuffer->contents + STAP_PACKET_HEADER_SIZE;
		payload.length = pbuffer->length - STAP_PACKET_HEADER_SIZE;
		// Something unknown?
	} else {
		FPRINTF("Unknown packet format: %u", pkthdr->format);
		// TODO: reset state
		goto bump_seq;
	}

	// We should only have one opt_len at the moment
	if (pkthdr->opt_len != 1) {
		FPRINTF("Invalid opt_len %u in packet header should be 1, DROPPING!", pkthdr->opt_len);
		goto bump_seq;
	}

	// Count how many packet headers we've processed
	int packet_header_num = 1;
	int payload_pos = 0;

	// Loop while we have data
	while (payload_pos < payload.length) {
		DEBUG_PRINT("Decoder loop: payload.length=%lu, payload_pos=%i, packet_header_num=%u", payload.length, payload_pos,
					packet_header_num);

		// Make sure we have space to overlay the optional packet header
		if (payload.length - payload_pos <= STAP_PACKET_PAYLOAD_HEADER_SIZE) {
			FPRINTF("Invalid payload length %lu, should be > %i to fit header %i, DROPPING!", payload.length,
					STAP_PACKET_PAYLOAD_HEADER_SIZE, packet_header_num);
			goto bump_seq;
		}

		// Overlay packet payload header
		pktphdr = (PacketPayloadHeaderComplete *) payload.contents + payload_pos;
		// Bump the payload contents pointer
		payload_pos += STAP_PACKET_PAYLOAD_HEADER_SIZE;
		DEBUG_PRINT("Moved payload_pos=%i past header", payload_pos);

		// If this is a normal encapsulated packet we can handle it here
		if (pktphdr->type == STAP_PACKET_PAYLOAD_TYPE_COMPLETE_PACKET) {
			// Make accessing wbuffer a bit easier
			Buffer *wbuffer = wbuffer_node->buffer;
			uint16_t payload_size = stap_be_to_cpu_16(pktphdr->payload_size);

			// Sanity check of options
			if (payload_size > payload.length - payload_pos) {
				FPRINTF("Packet size %u exceeds available payload length of %lu in header %i, DROPPING!", payload_size,
						payload.length - payload_pos, packet_header_num);
				goto bump_seq;
			}
			// Packets should be at least ETHERNET_FRAME_SIZE bytes in size
			if (payload_size < ETHERNET_FRAME_SIZE) {
				FPRINTF("Packet size %u is too small to be an ethernet frame in header %i, DROPPING!", payload_size,
						packet_header_num);
				goto bump_seq;
			}
			// Do a few sanity checks on the packet
			if (payload_size > STAP_MAX_PACKET_SIZE) {
				FPRINTF("Packet size %i exceeds maximum supported size %i, DROPPING!", payload_size, STAP_MAX_PACKET_SIZE);
				goto bump_seq;
			}
			if (payload_size > mtu + ETHERNET_FRAME_SIZE) {
				FPRINTF("DROPPING PACKET! packet size %i bigger than MTU %i, DROPPING!", payload_size, mtu);
				goto next_packet;
			}

			// Copy payload into the wbuffer
			memcpy(wbuffer->contents, payload.contents + payload_pos, payload_size);
			payload_pos += payload_size;
			wbuffer->length = payload_size;
			DEBUG_PRINT("Dumped packet to wbuffer, payload_pos=%u, wbuffer->length=%lu", payload_pos, wbuffer->length);

			// Next buffer...
			wbuffer_node = wbuffer_node->next;
			wbuffer_count++;
		} else {
			// If we don't understand what type of packet this is, drop it
			FPRINTF("I don't understand the payload header type %u, DROPPING!", pktphdr->type);
			goto bump_seq;
		}

	next_packet:
		// Bump header number
		packet_header_num += 1;
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
