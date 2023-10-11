
#include "packet.h"

#include <assert.h>
#include <linux/if_ether.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "checksum.h"
#include "debug.h"
#include "endian.h"
#include "util.h"

void _realloc_packet(ethernet_packet_t** pkt, uint16_t* pkt_size, uint16_t pkt_size_needed) {
	if (!*pkt_size) {
		*pkt = (ethernet_packet_t*)malloc(pkt_size_needed);
		*pkt_size = pkt_size_needed;
	} else if (*pkt_size < pkt_size_needed) {
		*pkt = (ethernet_packet_t*)realloc(*pkt, pkt_size_needed);
		*pkt_size = pkt_size_needed;
	}
}

/**
 * @brief Add an ethernet header to a packet.
 *
 * @param pkt Pointer to the packet pointer.
 * @param pkt_size Pointer to the packet size, this will be updated when the header is added.
 * @param dst_mac Destination MAC address.
 * @param src_mac Source MAC address.
 * @param ethertype Ethernet type.
 * @return Returns 0 on success or -1 on error.
 */
int add_packet_ethernet_header(ethernet_packet_t** pkt, uint16_t* pkt_size, uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN],
							   uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN], uint16_t ethertype) {
	// Make sure packet is big enough
	_realloc_packet(pkt, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN);

	memcpy((*pkt)->dst_mac, dst_mac, SETH_PACKET_ETHERNET_MAC_LEN);
	memcpy((*pkt)->src_mac, src_mac, SETH_PACKET_ETHERNET_MAC_LEN);
	(*pkt)->ethertype = seth_cpu_to_be_16(ethertype);

	return 0;
}

/**
 * @brief Add IPv4 packet header to an ethernet packet.
 *
 * @param pkt Pointer to the packet pointer.
 * @param pkt_size Pointer to the packet size, this will be updated when the header is added.
 * @param id Packet ID.
 * @param src_addr Source IPv4 address, represented as an array of 4 uint8_t's.
 * @param dst_addr Destination IPv4 address, represented as an array of 4 uint8_t's.
 * @return Returns 0 on success or -1 on error.
 */
int add_packet_ipv4_header(ethernet_packet_t** pkt, uint16_t* pkt_size, uint16_t id, uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN],
						   uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN]) {

	// Make sure packet is big enough
	_realloc_packet(pkt, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);

	ipv4_header_t* ipv4_packet = (ipv4_header_t*)&(*pkt)->payload;

	// Ensure the ethertype is correctly set
	(*pkt)->ethertype = seth_cpu_to_be_16(SETH_PACKET_TYPE_ETHERNET_IPV4);

	ipv4_packet->version = SETH_PACKET_IP_VERSION_IPV4;
	ipv4_packet->ihl = 5;
	ipv4_packet->dscp = 0;
	ipv4_packet->ecn = 0;
	ipv4_packet->total_length = 0;

	ipv4_packet->id = seth_cpu_to_be_16(id - 1);

	ipv4_packet->ttl = 64;
	ipv4_packet->protocol = 0;

	memcpy(ipv4_packet->src_addr, src_addr, SETH_PACKET_IPV4_IP_LEN);
	memcpy(ipv4_packet->dst_addr, dst_addr, SETH_PACKET_IPV4_IP_LEN);

	return 0;
}

/**
 * @brief Add a UDP header to an IP packet
 *
 * @param pkt Pointer to the packet pointer.
 * @param pkt_size Pointer to the packet size, this will be updated when the header is added.
 * @param src_port UDP source port.
 * @param dst_port UDP destination port.
 * @return Returns 0 on success or -1 on error.
 */
int add_packet_udp_header(ethernet_packet_t** pkt, uint16_t* pkt_size, uint16_t src_port, uint16_t dst_port) {
	// Grab ethertype
	uint16_t ethertype = seth_be_to_cpu_16((*pkt)->ethertype);

	// UDP packets with IPv4
	if (ethertype == SETH_PACKET_TYPE_ETHERNET_IPV4) {
		ipv4_header_t* ipv4_packet = (ipv4_header_t*)&(*pkt)->payload;

		// Make sure packet is big enough
		uint16_t header_len = ipv4_packet->ihl * 4;
		_realloc_packet(pkt, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN + header_len + SETH_PACKET_UDP_HEADER_LEN);

		udp_header_t* udp_packet = (udp_header_t*)(&(ipv4_packet->payload[ipv4_packet->ihl - 5]));

		ipv4_packet->protocol = SETH_PACKET_IP_PROTOCOL_UDP;

		udp_packet->src_port = seth_cpu_to_be_16(src_port);
		udp_packet->dst_port = seth_cpu_to_be_16(dst_port);
		udp_packet->checksum = 0;

	} else {
		DEBUG_PRINT("ERROR: Unsupported ethertype '0x%04X'", (*pkt)->ethertype);
		return -1;
	}

	return 0;
}

/**
 * @brief Add a ICMP header to an IP packet
 *
 * @param pkt Pointer to the packet pointer.
 * @param pkt_size Pointer to the packet size, this will be updated when the header is added.
 * @param type ICMP type.
 * @param code ICMP code.
 * @return Returns 0 on success or -1 on error.
 */
int add_packet_icmp_header(ethernet_packet_t** pkt, uint16_t* pkt_size, uint8_t type, uint8_t code) {
	// Grab ethertype
	uint16_t ethertype = seth_be_to_cpu_16((*pkt)->ethertype);

	// ICMP packets with IPv4
	if (ethertype == SETH_PACKET_TYPE_ETHERNET_IPV4) {
		ipv4_header_t* ipv4_packet = (ipv4_header_t*)&(*pkt)->payload;

		// Make sure packet is big enough
		uint16_t header_len = ipv4_packet->ihl * 4;
		_realloc_packet(pkt, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN + header_len + SETH_PACKET_ICMP_HEADER_LEN);

		icmp_header_t* icmp_packet = (icmp_header_t*)(&(ipv4_packet->payload[ipv4_packet->ihl - 5]));

		ipv4_packet->protocol = SETH_PACKET_IP_PROTOCOL_ICMP4;
		icmp_packet->type = type;
		icmp_packet->code = code;
		icmp_packet->checksum = 0;

	} else {
		DEBUG_PRINT("ERROR: Unsupported ethertype '0x%04X'", (*pkt)->ethertype);
		return -1;
	}

	return 0;
}

/**
 * @brief Add a ICMP ECHO header to an IP packet
 *
 * @param pkt Pointer to the packet pointer.
 * @param pkt_size Pointer to the packet size, this will be updated when the header is added.
 * @param identifier ICMP identifier
 * @param sequence ICMP sequence
 * @return Returns 0 on success or -1 on error.
 */
int add_packet_icmp_echo_header(ethernet_packet_t** pkt, uint16_t* pkt_size, uint16_t identifier, uint16_t sequence) {
	// Grab ethertype
	uint16_t ethertype = seth_be_to_cpu_16((*pkt)->ethertype);

	// ICMP packets with IPv4
	if (ethertype == SETH_PACKET_TYPE_ETHERNET_IPV4) {
		ipv4_header_t* ipv4_packet = (ipv4_header_t*)&(*pkt)->payload;

		// Make sure packet is big enough
		uint16_t header_len = ipv4_packet->ihl * 4;
		_realloc_packet(pkt, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN + header_len + SETH_PACKET_ICMP_ECHO_HEADER_LEN);

		icmp_echo_header_t* icmp_packet = (icmp_echo_header_t*)(&(ipv4_packet->payload[ipv4_packet->ihl - 5]));

		ipv4_packet->protocol = SETH_PACKET_IP_PROTOCOL_ICMP4;
		icmp_packet->type = 8;
		icmp_packet->code = 0;
		icmp_packet->checksum = 0;
		icmp_packet->identifier = seth_cpu_to_be_16(identifier);
		icmp_packet->sequence = seth_cpu_to_be_16(sequence);

	} else {
		DEBUG_PRINT("ERROR: Unsupported ethertype '0x%04X'", (*pkt)->ethertype);
		return -1;
	}

	return 0;
}

/**
 * @brief Set up packet length fields to correct values.
 *
 * @param pkt Pointer to the packet pointer.
 * @param pkt_size Pointer to the packet size, this will be updated when the header is added.
 * @return Returns 0 on success or -1 on error.
 */
int set_length(ethernet_packet_t** pkt, uint16_t pkt_size) {
	// Grab ethertype
	uint16_t ethertype = seth_be_to_cpu_16((*pkt)->ethertype);

	// UDP packets with IPv4
	if (ethertype == SETH_PACKET_TYPE_ETHERNET_IPV4) {
		// Check ethernet frame header
		uint16_t header_len = SETH_PACKET_ETHERNET_HEADER_LEN;
		if (pkt_size < header_len) {
			DEBUG_PRINT("[MALFORMED ETHERNET PACKET: size %u < %u]", pkt_size, header_len);
			return -1;
		}
		// Check IP header
		header_len = SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IP_HEADER_LEN;
		if (pkt_size < header_len) {
			DEBUG_PRINT("[MALFORMED IP PACKET: size %u < %u]", pkt_size, header_len);
			return -1;
		}
		ip_header_t* ip_header = (ip_header_t*)&(*pkt)->payload;

		// This is the protocol to use, as it comes from different IP versions, we need to set it here
		uint8_t* protocol;
		// This is the end of the IP header (4 or 6) payload
		uint8_t* ip_payload;
		// Start loc off at the ethernet header len
		uint16_t loc = SETH_PACKET_ETHERNET_HEADER_LEN;
		// Based on the IP header, work out the protocol version used
		if (ip_header->version == SETH_PACKET_IP_VERSION_IPV4) {
			uint16_t header_len = SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN;
			if (pkt_size < header_len) {
				DEBUG_PRINT("[MALFORMED IPV4 PACKET: size %u < %u]", pkt_size, header_len);
				return -1;
			}

			// Overlay IPv4 header
			ipv4_header_t* ipv4_packet = (ipv4_header_t*)ip_header;

			// Work out header length
			header_len = SETH_PACKET_ETHERNET_HEADER_LEN + (ipv4_packet->ihl * 4);
			if (pkt_size < header_len) {
				DEBUG_PRINT("[MALFORMED IPV4 PACKET: size %u < %u(ihl*4)]", pkt_size, header_len);
				return -1;
			}

			ipv4_packet->total_length = seth_cpu_to_be_16(pkt_size - SETH_PACKET_ETHERNET_HEADER_LEN);
			// Set the protocol we're using
			protocol = &ipv4_packet->protocol;
			// Start of IP payload
			ip_payload = (uint8_t*)&ipv4_packet->payload[ipv4_packet->ihl - 5];
			// Bump current loc to past the IPv4 header
			loc += ipv4_packet->ihl * 4;

		} else {
			DEBUG_PRINT("ERROR: Protocol 0x%04X not supported yet", ip_header->version);
			return -1;
		}
		// Check for protocol-specific checksums
		if (*protocol == SETH_PACKET_IP_PROTOCOL_UDP) {
			udp_header_t* udp_packet = (udp_header_t*)ip_payload;
			// Set protocol length
			udp_packet->length = seth_cpu_to_be_16(pkt_size - loc);
		}
	} else {
		DEBUG_PRINT("ERROR: Unsupported ethertype '0x%04X'", (*pkt)->ethertype);
		return -1;
	}

	return 0;
}

/**
 * @brief Set up packet checksum fields to correct values.
 *
 * @param pkt Pointer to the packet pointer.
 * @param pkt_size Pointer to the packet size, this will be updated when the header is added.
 * @return Returns 0 on success or -1 on error.
 */
int set_checksums(ethernet_packet_t** pkt, uint16_t pkt_size) {
	if (!is_valid_ethernet_ip(*pkt, pkt_size)) return -1;

	ip_header_t* ip_header = (ip_header_t*)&(*pkt)->payload;

	// This is the protocol to use, as it comes from different IP versions, we need to set it here
	uint8_t* protocol;
	// This is the end of the IP header (4 or 6) payload
	uint8_t* ip_payload;
	// Based on the IP header, work out the protocol version used
	if (ip_header->version == SETH_PACKET_IP_VERSION_IPV4) {
		if (!is_valid_ethernet_ipv4(*pkt, pkt_size)) return -1;

		// Overlay IPv4 header
		ipv4_header_t* ipv4_packet = (ipv4_header_t*)ip_header;

		// Set IPv4 checksum on the headers
		ipv4_packet->checksum = 0;
		uint16_t checksum = compute_ipv4_header_checksum((ethernet_packet_t*)*pkt, pkt_size);
		ipv4_packet->checksum = seth_cpu_to_be_16(checksum);

		// Set the protocol we're using
		protocol = &ipv4_packet->protocol;
		// Start of IP payload
		ip_payload = (uint8_t*)&ipv4_packet->payload[ipv4_packet->ihl - 5];

	} else {
		DEBUG_PRINT("ERROR: IP version 0x%04X not supported yet", ip_header->version);
		return -1;
	}

	// Check for protocol-specific checksums
	if (*protocol == SETH_PACKET_IP_PROTOCOL_UDP) {
		if (!is_valid_ethernet_ipv4_udp(*pkt, pkt_size)) return -1;
		udp_header_t* udp_packet = (udp_header_t*)ip_payload;
		// Calculate UDP checksum
		udp_packet->checksum = 0;
		udp_packet->checksum = seth_cpu_to_be_16(compute_ipv4_udp_checksum(*pkt, pkt_size));
	} else if (*protocol == SETH_PACKET_IP_PROTOCOL_ICMP4) {
		if (!is_valid_ethernet_ipv4_icmp(*pkt, pkt_size)) return -1;
		icmp_header_t* icmp_packet = (icmp_header_t*)ip_payload;
		// Calculate ICMP checksum
		icmp_packet->checksum = 0;
		icmp_packet->checksum = seth_cpu_to_be_16(compute_ipv4_icmp_checksum(*pkt, pkt_size));
	} else {
		DEBUG_PRINT("ERROR: Protocol 0x%04X not supported yet", *protocol);
		return -1;
	}

	return 0;
}

/**
 * @brief Add payload to packet.
 *
 * @param pkt Pointer to the packet pointer.
 * @param pkt_size Pointer to the packet size, this will be updated when the header is added.
 * @param payload Payload data.
 * @param payload_size Size of payload data.
 * @return Returns 0 on success or -1 on error.
 */
int add_payload(ethernet_packet_t** pkt, uint16_t* pkt_size, uint8_t* payload, uint16_t payload_size) {
	if (!is_valid_ethernet_ip(*pkt, *pkt_size)) return -1;

	ip_header_t* ip_header = (ip_header_t*)&(*pkt)->payload;

	// This is the protocol to use, as it comes from different IP versions, we need to set it here
	uint8_t* protocol;
	// This is the end of the IP header (4 or 6) payload
	uint8_t* ip_payload;
	// Based on the IP header, work out the protocol version used
	if (ip_header->version == SETH_PACKET_IP_VERSION_IPV4) {
		if (!is_valid_ethernet_ipv4(*pkt, *pkt_size)) return -1;

		// Overlay IPv4 header
		ipv4_header_t* ipv4_packet = (ipv4_header_t*)ip_header;

		// Set the protocol we're using
		protocol = &ipv4_packet->protocol;
		ip_payload = (uint8_t*)&ipv4_packet->payload[ipv4_packet->ihl - 5];

	} else {
		DEBUG_PRINT("ERROR: IP version 0x%04X not supported yet", ip_header->version);
		return -1;
	}

	// Check for protocol-specific handling of payloads
	if (*protocol == SETH_PACKET_IP_PROTOCOL_UDP) {
		uint16_t header_len = is_valid_ethernet_ipv4_udp(*pkt, *pkt_size);
		if (!header_len) return -1;

		udp_header_t* udp_packet = (udp_header_t*)ip_payload;

		// Bump loc with the UDP header length and re-allocate memory
		_realloc_packet(pkt, pkt_size, header_len + payload_size);

		// Copy in payload
		memcpy(udp_packet->payload, payload, payload_size);

	} else if (*protocol == SETH_PACKET_IP_PROTOCOL_ICMP4) {
		uint16_t header_len = is_valid_ethernet_ipv4_udp(*pkt, *pkt_size);
		if (!header_len) return -1;

		icmp_header_t* icmp_packet = (icmp_header_t*)ip_payload;

		// Bump loc with the UDP header length and re-allocate memory
		_realloc_packet(pkt, pkt_size, header_len + payload_size);

		// Copy in payload
		memcpy(icmp_packet->payload, payload, payload_size);
	} else {
		DEBUG_PRINT("ERROR: Protocol 0x%04X not supported yet", ip_header->protocol);
		return -1;
	}

	return 0;
}

static uint64_t _compute_ipv4_pseudo_checksum_partial(ethernet_packet_t* pkt, seth_be16_t layer4_length_be) {
	/* The IPv4 pseudo-header is defined in RFC 793, Section 3.1. */
	struct ipv4_pseudo_header_t {
		uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN];
		uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN];
		uint8_t zero;
		uint8_t protocol;
		seth_be16_t length;
	} __seth_packed;

	struct ipv4_pseudo_header_t pseudo_header;

	ip_header_t* ip_header = (ip_header_t*)&(pkt->payload);
	assert(ip_header->version == 4);

	ipv4_header_t* ipv4_packet = (ipv4_header_t*)ip_header;

	/* Fill in the pseudo-header. */
	memcpy(&pseudo_header.src_addr, &ipv4_packet->src_addr, sizeof(ipv4_packet->src_addr));
	memcpy(&pseudo_header.dst_addr, &ipv4_packet->dst_addr, sizeof(ipv4_packet->dst_addr));
	pseudo_header.zero = 0;
	pseudo_header.protocol = ipv4_packet->protocol;
	pseudo_header.length = layer4_length_be;
	return compute_checksum_partial((uint8_t*)&pseudo_header, sizeof(pseudo_header), 0);
}

uint16_t compute_ipv4_header_checksum(ethernet_packet_t* pkt, uint16_t length) {
	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IP_HEADER_LEN);
	ip_header_t* ip_header = (ip_header_t*)&(pkt->payload);
	assert(ip_header->version == 4);  // Must be IPv4

	assert(length >= SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);
	ipv4_header_t* ipv4_packet = (ipv4_header_t*)ip_header;
	assert(ipv4_packet->ihl >= 5);	// Minimum of 5 headers must be present

	uint16_t ip_header_size = ipv4_packet->ihl * 4;	 // Header size
	assert(length >= SETH_PACKET_ETHERNET_HEADER_LEN + ip_header_size);

	ipv4_header_t tmp;
	memcpy(&tmp, ipv4_packet, ip_header_size);
	tmp.checksum = 0;
	uint32_t partial_checksum = compute_checksum_partial((uint8_t*)&tmp, ip_header_size, 0);
	uint16_t checksum = compute_checksum_finalize(partial_checksum);

	return checksum;
}

uint16_t compute_ipv4_udp_checksum(ethernet_packet_t* pkt, uint16_t length) {
	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IP_HEADER_LEN);
	ip_header_t* ip_header = (ip_header_t*)&(pkt->payload);
	assert(ip_header->version == 4);  // Must be IPv4

	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);
	ipv4_header_t* ipv4_packet = (ipv4_header_t*)ip_header;
	assert(ipv4_packet->ihl >= 5);								   // Minimum of 5 headers must be present
	assert(ipv4_packet->protocol == SETH_PACKET_IP_PROTOCOL_UDP);  // Must match UDP protocol

	uint16_t ip_header_count = ipv4_packet->ihl - 5;  // Number of headers
	uint16_t ip_header_size = ipv4_packet->ihl * 4;	  // Header size

	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + ip_header_size);
	udp_header_t* udp_packet = (udp_header_t*)&(ipv4_packet->payload[ip_header_count]);

	// Grab our own UDP payload size as we obviously cannot read past the end of the buffer
	uint16_t udp_size = length - SETH_PACKET_ETHERNET_HEADER_LEN - ip_header_size;

	// Start off with the psudo header
	uint32_t partial_checksum = _compute_ipv4_pseudo_checksum_partial(pkt, seth_cpu_to_be_16(udp_size));

	// Copy packet, blank checksum, and dump payload into checksum
	udp_header_t tmp;
	memcpy(&tmp, udp_packet, SETH_PACKET_UDP_HEADER_LEN);
	tmp.checksum = 0;
	partial_checksum = compute_checksum_partial((uint8_t*)&tmp, SETH_PACKET_UDP_HEADER_LEN, partial_checksum);
	partial_checksum =
		compute_checksum_partial((uint8_t*)&udp_packet->payload, udp_size - SETH_PACKET_UDP_HEADER_LEN, partial_checksum);

	// Finally finalize the checksum
	uint16_t checksum = compute_checksum_finalize(partial_checksum);

	return checksum;
}

uint16_t compute_ipv4_icmp_checksum(ethernet_packet_t* pkt, uint16_t length) {
	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IP_HEADER_LEN);
	ip_header_t* ip_header = (ip_header_t*)&(pkt->payload);
	assert(ip_header->version == 4);  // Must be IPv4

	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);
	ipv4_header_t* ipv4_packet = (ipv4_header_t*)ip_header;
	assert(ipv4_packet->ihl >= 5);								   // Minimum of 5 headers must be present
	assert(ipv4_packet->protocol == SETH_PACKET_IP_PROTOCOL_ICMP4);  // Must match UDP protocol

	uint16_t ip_header_count = ipv4_packet->ihl - 5;  // Number of headers
	uint16_t ip_header_size = ipv4_packet->ihl * 4;	  // Header size

	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + ip_header_size);
	icmp_header_t* icmp_packet = (icmp_header_t*)&(ipv4_packet->payload[ip_header_count]);

	// Grab our own UDP payload size as we obviously cannot read past the end of the buffer
	uint16_t icmp_size = length - SETH_PACKET_ETHERNET_HEADER_LEN - ip_header_size;


	// Copy packet, blank checksum, and dump payload into checksum
	icmp_header_t tmp;
	memcpy(&tmp, icmp_packet, SETH_PACKET_ICMP_HEADER_LEN);
	tmp.checksum = 0;
	uint32_t partial_checksum = compute_checksum_partial((uint8_t*)&tmp, SETH_PACKET_ICMP_HEADER_LEN, 0);
	partial_checksum =
		compute_checksum_partial((uint8_t*)&icmp_packet->payload, icmp_size - SETH_PACKET_ICMP_HEADER_LEN, partial_checksum);


	// Finally finalize the checksum
	uint16_t checksum = compute_checksum_finalize(partial_checksum);

	return checksum;
}


int is_valid_ethernet(const ethernet_packet_t* packet, uint16_t size) {
	uint16_t header_len = SETH_PACKET_ETHERNET_HEADER_LEN;
	if (size < header_len) {
		DEBUG_PRINT("[MALFORMED ETHERNET PACKET: size %i < %i]", size, header_len);
		return 0;
	}
	return header_len;
}

int is_valid_ethernet_ip(const ethernet_packet_t* packet, uint16_t size) {
	uint16_t header_len = is_valid_ethernet(packet, size);
	if (!header_len) return 0;

	if (size < header_len + SETH_PACKET_IP_HEADER_LEN) {
		DEBUG_PRINT("[MALFORMED IP PACKET: size %i < %i]", size, header_len);
		return 0;
	}
	return header_len;
}

int is_valid_ethernet_ipv4(const ethernet_packet_t* packet, uint16_t size) {
	uint16_t header_len = is_valid_ethernet_ip(packet, size);
	if (!header_len) return 0;

	// Minimum header size check
	if (size < header_len + SETH_PACKET_IPV4_HEADER_LEN) {
		DEBUG_PRINT("[MALFORMED IPV4 PACKET: size %i < %i]", size, header_len);
		return 0;
	}

	const ipv4_header_t* ipv4_packet = (ipv4_header_t*)&packet->payload;

	if (ipv4_packet->ihl < 5) {
		DEBUG_PRINT("[MALFORMED IPV4 PACKET: ihl %u < 5]", ipv4_packet->ihl);
		return 0;
	}

	header_len += (ipv4_packet->ihl * 4);
	if (size < header_len) {
		DEBUG_PRINT("[MALFORMED IPV4 PACKET: size %i < %i(ihl*4)]", size, header_len);
		return 0;
	}
	return header_len;
}

int is_valid_ethernet_ipv4_udp(const ethernet_packet_t* packet, uint16_t size) {
	uint16_t header_len = is_valid_ethernet_ipv4(packet, size);
	if (!header_len) return 0;

	header_len += SETH_PACKET_UDP_HEADER_LEN;
	if (size < header_len) {
		DEBUG_PRINT("[MALFORMED UDP PACKET: size %i < %i]", size, header_len);
		return 0;
	}

	return header_len;
}

int is_valid_ethernet_ipv4_icmp(const ethernet_packet_t* packet, uint16_t size) {
	uint16_t header_len = is_valid_ethernet_ipv4(packet, size);
	if (!header_len) return 0;

	header_len += SETH_PACKET_ICMP_HEADER_LEN;
	if (size < header_len) {
		DEBUG_PRINT("[MALFORMED ICMP PACKET: size %i < %i]", size, header_len);
		return 0;
	}

	const icmp_header_t* icmp_packet = (icmp_header_t*)&packet->payload;
	if (icmp_packet->type == 8) {
		return header_len;

	} else {
		DEBUG_PRINT("[UKNOWN ICMP PACKET: type=0x%02X, code=0x%02X]", icmp_packet->type, icmp_packet->code);
	}

	return header_len;
}