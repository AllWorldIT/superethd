#include "packet.h"

#include <assert.h>
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
	state->payload_pos = 0;	  // Set to 0 on new payload
	state->payload_part = 0;  // Set to 0 on new payload
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
	PacketPayloadHeader *pktphdr;

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
		state->payload_part = 0;
	}

	// If we don't have a write buffer it means we flushed the previous one(s) and we need another
	// Clearing of wbuffer_node is done in reset_packet_encoder(), normally after wbuffers flush
	if (!state->wbuffer_node) {
		state->wbuffer_node = wbuffers->head;
		state->wbuffer_node->buffer->length = 0;
		state->wbuffer_count = 0;
	}

	// We're going to fill the packet up at least until we have 128 bytes left at the end
	Buffer *wbuffer = state->wbuffer_node->buffer;

	// Loop until we've processed the payload
	while (state->payload_pos < state->payload->length) {
		int chunk_size = 0;
		int payload_header_type = 0;

		// Work out our header size, at the bare minimum we need the payload header
		int header_size = STAP_PACKET_PAYLOAD_HEADER_SIZE;
		// If our length is 0, it means its a new buffer, so we also nead the packet header
		if (!wbuffer->length) header_size += STAP_PACKET_HEADER_SIZE;
		DEBUG_PRINT("header_size%i", header_size);

		// Work out wbuffer space left
		int wbuffer_left = max_wbuffer_size - wbuffer->length - header_size;
		DEBUG_PRINT("wbuffer_left=%i  (mss=%i - wbuffer->length=%lu - header_size=%i)", wbuffer_left, max_wbuffer_size,
					wbuffer->length, header_size);

		// Work out payload data left
		int payload_left = state->payload->length - state->payload_pos;
		DEBUG_PRINT("payload_left=%i", payload_left);

		// Check if this is the first packet and if the entire thing will fit
		if (!state->payload_part && payload_left < wbuffer_left) {	// Packet will fit
			chunk_size = state->payload->length;
			payload_header_type = STAP_PACKET_PAYLOAD_TYPE_COMPLETE_PACKET;
			DEBUG_PRINT("chunk_size=%i (payload_left=%i < wbuffer_left=%i) - PACKET FITS", chunk_size, payload_left, wbuffer_left);
		} else {  // Packet won't fit, so we try use the smallest of either the wbuffer_left or payload_left
			payload_header_type = STAP_PACKET_PAYLOAD_TYPE_PARTIAL_PACKET;
			// Bump the header size and reduce wbuffer_left, this header is bigger
			header_size += STAP_PACKET_PAYLOAD_HEADER_PARTIAL_SIZE;
			wbuffer_left -= STAP_PACKET_PAYLOAD_HEADER_PARTIAL_SIZE;
			chunk_size = (payload_left < wbuffer_left) ? payload_left : wbuffer_left;
			DEBUG_PRINT("chunk_size=%i (payload_left=%i, new wbuffer_left=%i, new header_size=%i)", chunk_size, payload_left,
						wbuffer_left, header_size);
		}

		// If the length is zero, it means we need a packet header
		if (!wbuffer->length) {
			// Set up packet header, we point it to the buffer
			pkthdr = (PacketHeader *)wbuffer->contents;
			DEBUG_PRINT("NEW Header @wbuffer->length=%lu", wbuffer->length);
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
			DEBUG_PRINT("bumping wbuffer->length=%lu after pkthdr setup (first header in new packet)", wbuffer->length);
		}

		// Write the packet payload header
		pktphdr = (PacketPayloadHeader *)(wbuffer->contents + wbuffer->length);
		pktphdr->type = payload_header_type;
		pktphdr->packet_size = stap_cpu_to_be_16(state->payload->length);
		pktphdr->reserved = 0;
		DEBUG_PRINT("NEW payload header @wbuffer->length=%lu, type=%i, packet_size=%i", wbuffer->length, pktphdr->type,
					pktphdr->packet_size);
		wbuffer->length += STAP_PACKET_PAYLOAD_HEADER_SIZE;

		// If this is a partial header, we overlay the partial header struct and set the additional values
		if (payload_header_type == STAP_PACKET_PAYLOAD_TYPE_PARTIAL_PACKET) {
			PacketPayloadHeaderPartial *pktphdr_partial = (PacketPayloadHeaderPartial *)(wbuffer->contents + wbuffer->length);

			pktphdr_partial->payload_length = stap_cpu_to_be_16(chunk_size);
			pktphdr_partial->part = state->payload_part;
			pktphdr_partial->reserved = 0;
			wbuffer->length += STAP_PACKET_PAYLOAD_HEADER_PARTIAL_SIZE;
			DEBUG_PRINT("Partial packet, adding partial packet header: payload_length=%i, payload_part=%i, wbuffer->length=%lu",
					chunk_size, state->payload_part, wbuffer->length);
		}

		// Copy chunk into buffer
		memcpy(wbuffer->contents + wbuffer->length, state->payload->contents + state->payload_pos, chunk_size);
		// Bump all our states
		state->payload_pos += chunk_size;
		wbuffer->length += chunk_size;
		state->payload_part++;

		DEBUG_PRINT(
			"Dumped chunk to wbuffer: payload_pos=%i, chunk_size=%i, wbuffer->length=%lu, payload_part=%i : encap next seq=%i",
			state->payload_pos, chunk_size, wbuffer->length, state->payload_part, state->seq);

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
	// Packet counting and sequencing
	state->first_packet = 1;
	state->last_seq = 0;
	// Number of write buffers filled
	state->wbuffer_node = NULL;	 // Set to NULL once buffers are written
	state->wbuffer_count = 0;	 // Set to 0 on new encoding run
	// Allocate buffer used for decompression and assembly
	state->cbuffer.contents = (char *)malloc(STAP_BUFFER_SIZE_COMPRESSION);
	state->cbuffer.length = 0;

	// Clear and set up partial packet payload
	state->partial_packet.contents = (char *)malloc(STAP_MAX_PACKET_SIZE);
	state->partial_packet.length = 0;
	state->partial_packet_part = 0;
	state->partial_packet_lastseq = 0;

	return 0;
}

// Helper function to clear partial packet state
static inline void _clear_packet_decoder_partial_state(PacketDecoderState *state) {
	state->partial_packet.length = 0;
	state->partial_packet_part = 0;
	state->partial_packet_lastseq = 0;
	state->partial_packet_size = 0;
}

static int _decode_complete_packet(PacketDecoderState *state, PacketDecoderEncapPacket *epkt) {
	// Sanity check of options
	if (epkt->decoded_packet_size > epkt->payload.length - epkt->pos) {
		FPRINTF("Packet size %u exceeds available payload length of %lu in header %i, DROPPING!", epkt->decoded_packet_size,
				epkt->payload.length - epkt->pos, epkt->cur_header);
		return -1;
	}

	// Make accessing the write buffer a bit easier
	Buffer *wbuffer = state->wbuffer_node->buffer;

	// Copy payload into the wbuffer
	memcpy(wbuffer->contents, epkt->payload.contents + epkt->pos, epkt->decoded_packet_size);
	epkt->pos += epkt->decoded_packet_size;
	wbuffer->length = epkt->decoded_packet_size;
	DEBUG_PRINT("Dumped packet to wbuffer: payload_pos=%u, wbuffer->length=%lu : seq=%i", epkt->pos, wbuffer->length, epkt->seq);

	// Bump buffer counter
	state->wbuffer_count++;
	// If we've processed this payload, we can reset the buffer node as we're done here
	if (epkt->pos != epkt->payload.length) state->wbuffer_node = state->wbuffer_node->next;

	return 0;
}

static int _decode_partial_packet(PacketDecoderState *state, PacketDecoderEncapPacket *epkt) {
	// Overlay the partial packet header and move onto checking the partial packet below
	PacketPayloadHeaderPartial *pktphdr_partial = (PacketPayloadHeaderPartial *)(epkt->payload.contents + epkt->pos);
	// Bump the payload contents pointer past the partial header
	epkt->pos += STAP_PACKET_PAYLOAD_HEADER_PARTIAL_SIZE;
	DEBUG_PRINT("Moved payload_pos=%i past partial header", epkt->pos);

	if (state->partial_packet_size)
		DEBUG_PRINT("Re-assembling: part=%i, packet_size=%lu/%i, lastseq=%i", state->partial_packet_part,
					state->partial_packet.length, state->partial_packet_size, state->partial_packet_lastseq);

	// Make sure we have space to overlay the partial packet header
	if (epkt->payload.length - epkt->pos <= STAP_PACKET_PAYLOAD_HEADER_PARTIAL_SIZE * 2) {
		FPRINTF("Invalid payload length %lu, should be > %i to fit partial packet header %i, DROPPING!", epkt->payload.length,
				STAP_PACKET_PAYLOAD_HEADER_PARTIAL_SIZE, epkt->cur_header);
		// Something is pretty wrong here, so clear partial packet state and go to next encap packet
		_clear_packet_decoder_partial_state(state);
		return -1;
	}

	// Make sure its reserved field is also cleared
	if (pktphdr_partial->reserved != 0) {
		FPRINTF("Partial packet header should not have any reserved its set, it is %u, DROPPING!", pktphdr_partial->reserved);
		// Something is probably corrupted, so clear the state and continue to next encap packet
		_clear_packet_decoder_partial_state(state);
		return -1;
	}

	// Set the partial packet size
	uint16_t partial_packet_payload_length = stap_be_to_cpu_16(pktphdr_partial->payload_length);

	// Check that this partial packet size does not exceed the encap payload memory space left
	if (partial_packet_payload_length > epkt->payload.length - epkt->pos) {
		FPRINTF("Partial packet payload size %u exceeds available payload length of %lu in header %i, DROPPING!",
				epkt->decoded_packet_size, epkt->payload.length - epkt->pos, epkt->cur_header);
		// The partial packet size can never exceed the memory left in the payload, if this is true, something is wrong
		// clear the state and move to the next encap packet.
		_clear_packet_decoder_partial_state(state);
		return -1;
	}

	/*
	 * The below checks are based on this specific re-assembly, it could be we had packet loss, so we can probably
	 * continue onto the next packet header incase other packets were packed into this payload.
	 */

	// Check that the partial packet size matches what we got originally
	if (state->partial_packet_size && state->partial_packet_size != epkt->decoded_packet_size) {
		FPRINTF(
			"Partial packet size does not match the one in the first header state->partial_packet_size=%i, "
			"partial_packet_size=%i, DROPPING!",
			state->partial_packet_size, epkt->decoded_packet_size);
		// The original packet size we got does not match what we got now, this shouldn't happen, drop packet and
		// continue onto the next encap packet.
		_clear_packet_decoder_partial_state(state);
		epkt->pos += epkt->decoded_packet_size;
		return 0;
	}

	// Check encap packet sequence is in order
	if (state->partial_packet_lastseq && state->partial_packet_lastseq != epkt->seq - 1) {
		FPRINTF("Partial packet encap sequence mismatch partial_packet_lastseq=%i, (sequence - 1)=%i, DROPPING!",
				state->partial_packet_lastseq, epkt->seq - 1);
		// We obviously lost data, so reset the partial packet assembly
		_clear_packet_decoder_partial_state(state);
		epkt->pos += epkt->decoded_packet_size;
		return 0;
	}

	// Verify packet part is in numerical order
	if (state->partial_packet_size && state->partial_packet_part != pktphdr_partial->part - 1) {
		FPRINTF("Partial packet part mismatch partial_packet_part=%i, (partial_packet_part - 1)=%i, DROPPING!",
				state->partial_packet_part, pktphdr_partial->part - 1);
		// Payload part did not match what it should be, probably corruption?
		_clear_packet_decoder_partial_state(state);
		return -1;
	}

	// We can now make sure that our part starts at 1 aswell...
	if (!state->partial_packet_size && pktphdr_partial->part != 0) {
		FPRINTF("Partial packet part should start at 0? part=%i, DROPPING!", pktphdr_partial->part);
		// Odd, our packet part should start at 1, lets try the next packet
		_clear_packet_decoder_partial_state(state);
		epkt->pos += epkt->decoded_packet_size;
		return 0;
	}

	// Make sure if we add this payload, the packet will not exceed the size it said it was
	if (state->partial_packet.length + partial_packet_payload_length > epkt->decoded_packet_size) {
		FPRINTF("Partial packet size will exceed packet size in header %i with payload %i, DROPPING!", epkt->decoded_packet_size,
				partial_packet_payload_length);
		// This should never happen, so maybe its corruption? clear start and discard rest of encap packet
		_clear_packet_decoder_partial_state(state);
		return -1;
	}

	// Copy payload into the partial packet buffer
	memcpy(state->partial_packet.contents + state->partial_packet.length, epkt->payload.contents + epkt->pos,
		   partial_packet_payload_length);
	// Bump the positions of the encap and also partial packet
	epkt->pos += partial_packet_payload_length;
	// If we didn't catch the full packet, we need to update all the state below...
	state->partial_packet.length += partial_packet_payload_length;

	DEBUG_PRINT("Partial packet: part=%i, size=%i [payload: %i, cur=%lu] : encap seq=%i [encap pos: %i]", pktphdr_partial->part,
				epkt->decoded_packet_size, partial_packet_payload_length, state->partial_packet.length, epkt->seq, epkt->pos);

	/*
	 * Check if we got the full packet
	 */

	// Lets check if we have the full packet now...
	if (state->partial_packet.length == epkt->decoded_packet_size) {
		// Make accessing the write buffer a bit easier
		Buffer *wbuffer = state->wbuffer_node->buffer;

		// Copy payload into the wbuffer
		memcpy(wbuffer->contents, state->partial_packet.contents, state->partial_packet.length);
		wbuffer->length = state->partial_packet.length;
		DEBUG_PRINT("Dumped assembled partial packet to wbuffer, wbuffer->length=%lu", wbuffer->length);

		// Clear state for next packet
		_clear_packet_decoder_partial_state(state);

		// Bump buffer counter
		state->wbuffer_count++;
		state->wbuffer_node = state->wbuffer_node->next;

		return 0;
	}

	/*
	 * If we did not get the packet, we need to update the state
	 */

	// If this is the first partial packet, we can set its size
	if (pktphdr_partial->part == 0) state->partial_packet_size = epkt->decoded_packet_size;

	// Update the state for this partial payload
	state->partial_packet_lastseq = epkt->seq;
	state->partial_packet_part = pktphdr_partial->part;

	return 0;
}

/**
 * @brief Decode packets received over the tunnel.
 *
 * Decode raw packets received over the tunne by parsing the headers and dumping packets into wbuffers.
 *
 * @param state Decoder state.
 * @param wbuffers List of buffers used to write to TAP device.
 * @param pbuffer Packet buffer, this is the packet we're processing.
 * @param emtu Maximum MTU of our ethernet device.
 * @return Number of packets ready in the wbuffers.
 */
int packet_decoder(PacketDecoderState *state, BufferList *wbuffers, Buffer *pbuffer, uint16_t emtu) {
	// Packet we're working on
	PacketDecoderEncapPacket epkt;

	// Packet header pointer
	PacketHeader *pkthdr = (PacketHeader *)pbuffer->contents;
	PacketPayloadHeader *pktphdr;

	// Maximum MTU for resulting packet
	epkt.mtu = emtu;
	// Decode sequence
	epkt.seq = stap_be_to_cpu_32(pkthdr->sequence);

	// If this is not the first packet...
	if (!state->first_packet) {
		// Check if we've lost packets
		if (epkt.seq > state->last_seq + 1) {
			FPRINTF("PACKET LOST: last=%u, seq=%u, total_lost=%u", state->last_seq, epkt.seq, epkt.seq - state->last_seq);
		}
		// If we we have an out of order one
		if (epkt.seq < state->last_seq + 1) {
			FPRINTF("PACKET OOO : last=%u, seq=%u", state->last_seq, epkt.seq);
		}
	}

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
		epkt.payload.contents = state->cbuffer.contents;
		epkt.payload.length = state->cbuffer.length;
		// Plain uncompressed encapsulated packets
	} else if (pkthdr->format == STAP_PACKET_FORMAT_ENCAP) {
		// If the packet is not compressed, we just set the payload to point to the data following our packet header
		epkt.payload.contents = pbuffer->contents + STAP_PACKET_HEADER_SIZE;
		epkt.payload.length = pbuffer->length - STAP_PACKET_HEADER_SIZE;
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

	// Start at the beginning of wbuffers
	state->wbuffer_node = wbuffers->head;
	state->wbuffer_count = 0;

	// Count how many packet headers we've processed
	epkt.cur_header = 1;
	epkt.pos = 0;
	// Loop while we have data
	while (epkt.pos < epkt.payload.length) {
		DEBUG_PRINT("Decoder loop: epkt.payload.length=%lu, payload_pos=%i, header_num=%u", epkt.payload.length, epkt.pos,
					epkt.cur_header);

		// Make sure we have space to overlay the optional packet header
		if (epkt.payload.length - epkt.pos <= STAP_PACKET_PAYLOAD_HEADER_SIZE) {
			FPRINTF("Invalid payload length %lu, should be > %i to fit header %i, DROPPING!", epkt.payload.length,
					STAP_PACKET_PAYLOAD_HEADER_SIZE, epkt.cur_header);
			goto bump_seq;
		}

		// Overlay packet payload header
		pktphdr = (PacketPayloadHeader *)(epkt.payload.contents + epkt.pos);
		// Bump the payload contents pointer
		epkt.pos += STAP_PACKET_PAYLOAD_HEADER_SIZE;
		DEBUG_PRINT("Moved payload_pos=%i past header", epkt.pos);

		// This is the packet size of the final packet after reassembly
		epkt.decoded_packet_size = stap_be_to_cpu_16(pktphdr->packet_size);
		DEBUG_PRINT("Final packet size: decoded_packet_size=%i", epkt.decoded_packet_size);

		// Packets should be at least ETHERNET_FRAME_SIZE bytes in size
		if (epkt.decoded_packet_size < ETHERNET_FRAME_SIZE) {
			FPRINTF("Packet size %u is too small to be an ethernet frame in header %i, DROPPING!", epkt.decoded_packet_size,
					epkt.cur_header);
			// Something is probably pretty wrong here too, so lets again clear the state and continue to the next encap
			// packet
			_clear_packet_decoder_partial_state(state);
			return -1;
		}
		// Do a few sanity checks on the packet
		if (epkt.decoded_packet_size > STAP_MAX_PACKET_SIZE) {
			FPRINTF("Packet size %i exceeds maximum supported size %i, DROPPING!", epkt.decoded_packet_size, STAP_MAX_PACKET_SIZE);
			// Again, probably something pretty wrong here, this cannot be the case, so we discard the encap packet and
			// clear state
			_clear_packet_decoder_partial_state(state);
			return -1;
		}
		if (epkt.decoded_packet_size > epkt.mtu + ETHERNET_FRAME_SIZE) {
			FPRINTF("DROPPING PACKET! packet size %i bigger than MTU %i, DROPPING!", epkt.decoded_packet_size, epkt.mtu);
			// Things should be properly setup, so this should not be able to happen, clear state and contine to next encap
			// packet
			_clear_packet_decoder_partial_state(state);
			return -1;
		}

		/*
		 * Packet decoding
		 */
		int res;
		if (pktphdr->type == STAP_PACKET_PAYLOAD_TYPE_COMPLETE_PACKET) res = _decode_complete_packet(state, &epkt);
		// Partial packet handling
		else if (pktphdr->type == STAP_PACKET_PAYLOAD_TYPE_PARTIAL_PACKET)
			res = _decode_partial_packet(state, &epkt);
		else {
			// If we don't understand what type of packet this is, drop it
			FPRINTF("I don't understand the payload header type %u, DROPPING!", pktphdr->type);
			res = -1;
		}

		// Check result of decode operation
		if (res == -1) goto bump_seq;

		// Bump header number
		epkt.cur_header += 1;
		DEBUG_PRINT("End loop: epkt.pos=%i, epkt.cur_header=%i", epkt.pos, epkt.cur_header);
	}

bump_seq:
	// We are done with the encap_payload for now, and the running of this function, so we can clear wbuffer_node
	state->wbuffer_node = NULL;

	// If this was the first packet, reset the first packet indicator
	if (state->first_packet) {
		state->first_packet = 0;
		state->last_seq = epkt.seq;
		// We only bump the last sequence if its smaller than the one we're on
	} else if (state->last_seq < epkt.seq) {
		state->last_seq = epkt.seq;
	} else if (state->last_seq > epkt.seq && sequence_wrapping(epkt.seq, state->last_seq)) {
		state->last_seq = epkt.seq;
	}

	return state->wbuffer_count;
}
