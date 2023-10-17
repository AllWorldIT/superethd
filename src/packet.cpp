
#include "packet.hpp"

#include <ios>
#include <iostream>
#include <new>
#include <sstream>
#include <vector>

extern "C" {
#include <assert.h>
#include <linux/if_ether.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include "checksum.hpp"
#include "debug.hpp"
#include "endian.hpp"
#include "util.hpp"



/*

void _realloc_packet(ethernet_header_t **pkt, uint16_t *pkt_size, uint16_t pkt_size_needed) {
	if (!*pkt_size) {
		*pkt = (ethernet_header_t *)malloc(pkt_size_needed);
		*pkt_size = pkt_size_needed;
	} else if (*pkt_size < pkt_size_needed) {
		ethernet_header_t *new_pkt = (ethernet_header_t *)realloc(*pkt, pkt_size_needed);
		if (!new_pkt) {
			throw std::bad_alloc();
		}
		*pkt = new_pkt;
		*pkt_size = pkt_size_needed;
	}
}


ethernet_header_t *add_packet_ethernet_header(ethernet_header_t **pkt, uint16_t *pkt_size,
											  uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN],
											  uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN], uint16_t ethertype) {
	// Make sure packet is big enough
	_realloc_packet(pkt, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN);

	memcpy((*pkt)->dst_mac, dst_mac, SETH_PACKET_ETHERNET_MAC_LEN);
	memcpy((*pkt)->src_mac, src_mac, SETH_PACKET_ETHERNET_MAC_LEN);

	ethernet_header_t *ethernet_packet = *pkt;
	ethernet_packet->ethertype = seth_cpu_to_be_16(ethertype);

	return *pkt;
}


ipv4_header_t *add_packet_ipv4_header(ethernet_header_t **pkt, uint16_t *pkt_size, uint16_t id,
									  uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN], uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN]) {
	// Make sure packet is big enough
	_realloc_packet(pkt, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);

	ethernet_header_t *ethernet_packet = *pkt;
	ipv4_header_t *ipv4_packet = (ipv4_header_t *)(ethernet_packet + SETH_PACKET_ETHERNET_HEADER_LEN);

	// Ensure the ethertype is correctly set
	ethernet_packet->ethertype = seth_cpu_to_be_16(SETH_PACKET_TYPE_ETHERNET_IPV4);

	ipv4_packet->version = SETH_PACKET_IP_VERSION_IPV4;
	ipv4_packet->ihl = 5;
	ipv4_packet->dscp = 0;
	ipv4_packet->ecn = 0;
	ipv4_packet->total_length = 0;

	ipv4_packet->id = seth_cpu_to_be_16(id - 1);

	ipv4_packet->frag_off = 0;

	ipv4_packet->ttl = 64;
	ipv4_packet->protocol = 0;

	ipv4_packet->checksum = 0;

	memcpy(ipv4_packet->src_addr, src_addr, SETH_PACKET_IPV4_IP_LEN);
	memcpy(ipv4_packet->dst_addr, dst_addr, SETH_PACKET_IPV4_IP_LEN);

	return ipv4_packet;
}


udp_header_t *add_packet_udp_header(ethernet_header_t **pkt, uint16_t *pkt_size, uint16_t src_port, uint16_t dst_port) {
	// Grab ethertype
	uint16_t ethertype = seth_be_to_cpu_16((*pkt)->ethertype);

	// UDP packets with IPv4
	if (ethertype == SETH_PACKET_TYPE_ETHERNET_IPV4) {
		// NK: Safety to ensure we don't use anything pointing to our packet again below
		{
			ipv4_header_t *ipv4_packet = (ipv4_header_t *)&(*pkt)->payload;
			uint16_t header_len = ipv4_packet->ihl * 4;
			// Make sure packet is big enough
			_realloc_packet(pkt, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN + header_len + SETH_PACKET_UDP_HEADER_LEN);
		}

		// NK: NB!!!!!!! - We ABSOLUTELY MUST get the pointer again to the packet as we called realloc
		ipv4_header_t *ipv4_packet = (ipv4_header_t *)&(*pkt)->payload;
		udp_header_t *udp_packet = (udp_header_t *)(IPV4_PROTOCOL_OFFSET(ipv4_packet));

		ipv4_packet->protocol = SETH_PACKET_IP_PROTOCOL_UDP;

		udp_packet->src_port = seth_cpu_to_be_16(src_port);
		udp_packet->dst_port = seth_cpu_to_be_16(dst_port);
		udp_packet->checksum = 0;
		return udp_packet;

	} else {
		std::ostringstream oss;
		oss << "Unsupported ethertype " << std::hex << std::uppercase << (*pkt)->ethertype;
		throw PacketNotSupportedEception(oss.str());
	}
}


icmp_header_t *add_packet_icmp_header(ethernet_header_t **pkt, uint16_t *pkt_size, uint8_t type, uint8_t code) {
	// Grab ethertype
	uint16_t ethertype = seth_be_to_cpu_16((*pkt)->ethertype);

	// ICMP packets with IPv4
	if (ethertype == SETH_PACKET_TYPE_ETHERNET_IPV4) {
		// NK: Safety to ensure we don't use anything pointing to our packet again below
		{
			ipv4_header_t *ipv4_packet = (ipv4_header_t *)&(*pkt)->payload;
			uint16_t header_len = ipv4_packet->ihl * 4;
			// Make sure packet is big enough
			_realloc_packet(pkt, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN + header_len + SETH_PACKET_ICMP_HEADER_LEN);
		}
		// NK: NB!!!!!!! - We ABSOLUTELY MUST get the pointer again to the packet as we called realloc
		ipv4_header_t *ipv4_packet = (ipv4_header_t *)&(*pkt)->payload;
		icmp_header_t *icmp_packet = (icmp_header_t *)(IPV4_PROTOCOL_OFFSET(ipv4_packet));

		ipv4_packet->protocol = SETH_PACKET_IP_PROTOCOL_ICMP4;
		icmp_packet->type = type;
		icmp_packet->code = code;
		icmp_packet->checksum = 0;

		return icmp_packet;

	} else {
		std::ostringstream oss;
		oss << "Unsupported ethertype " << std::hex << std::uppercase << (*pkt)->ethertype;
		throw PacketNotSupportedEception(oss.str());
	}
}


void add_packet_icmp_echo_header(ethernet_header_t **pkt, uint16_t *pkt_size, uint16_t identifier, uint16_t sequence) {
	// Grab ethertype
	uint16_t ethertype = seth_be_to_cpu_16((*pkt)->ethertype);

	// ICMP packets with IPv4
	if (ethertype == SETH_PACKET_TYPE_ETHERNET_IPV4) {
		// We should safely be able to add the header, then change values below
		icmp_echo_header_t *icmp_packet = (icmp_echo_header_t *)add_packet_icmp_header(pkt, pkt_size, 8, 0);

		icmp_packet->identifier = seth_cpu_to_be_16(identifier);
		icmp_packet->sequence = seth_cpu_to_be_16(sequence);

	} else {
		std::ostringstream oss;
		oss << "Unsupported ethertype " << std::hex << std::uppercase << (*pkt)->ethertype;
		throw PacketNotSupportedEception(oss.str());
	}
}


void set_length(ethernet_header_t **pkt, uint16_t pkt_size) {
	// Grab ethertype
	uint16_t header_len = is_valid_ethernet(*pkt, pkt_size);
	ethernet_header_t *ethernet_packet = *pkt;
	uint16_t ethertype = seth_be_to_cpu_16(ethernet_packet->ethertype);

	// IPv4 packets
	if (ethertype == SETH_PACKET_TYPE_ETHERNET_IPV4) {
		// Check ethernet frame header
		header_len = is_valid_ethernet_ipv4(*pkt, pkt_size);

		ipv4_header_t *ipv4_packet = (ipv4_header_t *)((*pkt) + SETH_PACKET_ETHERNET_HEADER_LEN);

		ipv4_packet->total_length = seth_cpu_to_be_16(pkt_size - SETH_PACKET_ETHERNET_HEADER_LEN);

		// Check for protocol-specific checksums
		if (ipv4_packet->protocol == SETH_PACKET_IP_PROTOCOL_UDP) {
			header_len = is_valid_ethernet_ipv4_udp(*pkt, pkt_size);

			udp_header_t *udp_packet = (udp_header_t *)ipv4_packet->payload;
			// Set protocol length
			udp_packet->length = seth_cpu_to_be_16(pkt_size - header_len + SETH_PACKET_UDP_HEADER_LEN);
		}
	} else {
		std::ostringstream oss;
		oss << "Unsupported ethertype " << std::hex << std::uppercase << (*pkt)->ethertype;
		throw PacketNotSupportedEception(oss.str());
	}
}


void set_checksums(ethernet_header_t **pkt, uint16_t pkt_size) {
	is_valid_ethernet_ip(*pkt, pkt_size);
	// Grab IP header
	ip_header_t *ip_header = (ip_header_t *)((*pkt) + SETH_PACKET_ETHERNET_HEADER_LEN);

	// This is the protocol to use, as it comes from different IP versions, we need to set it here
	uint8_t layer3_protocol;
	// This is the end of the IP header (4 or 6) payload
	uint8_t *layer4_payload;
	// Based on the IP header, work out the protocol version used
	if (ip_header->version == SETH_PACKET_IP_VERSION_IPV4) {
		uint16_t header_len = is_valid_ethernet_ipv4(*pkt, pkt_size);

		// Overlay IPv4 header
		ipv4_header_t *ipv4_packet = (ipv4_header_t *)ip_header;

		// Set IPv4 checksum on the headers
		ipv4_packet->checksum = 0;
		uint16_t checksum = compute_ipv4_header_checksum((ethernet_header_t *)*pkt, pkt_size);
		ipv4_packet->checksum = seth_cpu_to_be_16(checksum);

		// Set the protocol we're using
		layer3_protocol = ((ipv4_header_t *)ip_header)->protocol;
		// Get the layer 4 payload
		layer4_payload = (uint8_t *)((*pkt) + header_len);

	} else {
		std::ostringstream oss;
		oss << "Unsupported IP version " << ip_header->version;
		throw PacketNotSupportedEception(oss.str());
	}

	// Check for protocol-specific checksums
	if (layer3_protocol == SETH_PACKET_IP_PROTOCOL_UDP) {
		is_valid_ethernet_ipv4_udp(*pkt, pkt_size);
		udp_header_t *udp_packet = (udp_header_t *)layer4_payload;
		// Calculate UDP checksum
		udp_packet->checksum = 0;
		udp_packet->checksum = seth_cpu_to_be_16(compute_ipv4_udp_checksum(*pkt, pkt_size));

	} else if (layer3_protocol == SETH_PACKET_IP_PROTOCOL_ICMP4) {
		is_valid_ethernet_ipv4_icmp(*pkt, pkt_size);
		icmp_header_t *icmp_packet = (icmp_header_t *)layer4_payload;
		// Calculate ICMP checksum
		icmp_packet->checksum = 0;
		icmp_packet->checksum = seth_cpu_to_be_16(compute_ipv4_icmp_checksum(*pkt, pkt_size));
	}
}

void add_payload(ethernet_header_t **pkt, uint16_t *pkt_size, uint8_t *payload, uint16_t payload_size) {
	is_valid_ethernet_ip(*pkt, *pkt_size);
	ip_header_t *ip_header = (ip_header_t *)&(*pkt)->payload;

	// This is the protocol to use, as it comes from different IP versions, we need to set it here
	uint8_t layer3_protocol;
	// Based on the IP header, work out the protocol version used
	if (ip_header->version == SETH_PACKET_IP_VERSION_IPV4) {
		is_valid_ethernet_ipv4(*pkt, *pkt_size);
		// Set the protocol we're using
		layer3_protocol = ((ipv4_header_t *)ip_header)->protocol;

	} else {
		std::ostringstream oss;
		oss << "Unsupported IP version " << ip_header->version;
		throw PacketNotSupportedEception(oss.str());
	}

	// Check for protocol-specific handling of payloads
	if (layer3_protocol == SETH_PACKET_IP_PROTOCOL_UDP) {
		uint16_t header_len = is_valid_ethernet_ipv4_udp(*pkt, *pkt_size);
		// Make sure we have enough memory for the payload
		_realloc_packet(pkt, pkt_size, header_len + payload_size);
		// NK: NB!!!! Realloc can change the memory address
		udp_header_t *udp_packet = (udp_header_t *)((*pkt) + header_len);
		// Copy in payload
		memcpy(&udp_packet->payload, payload, payload_size);

	} else if (layer3_protocol == SETH_PACKET_IP_PROTOCOL_ICMP4) {
		uint16_t header_len = is_valid_ethernet_ipv4_icmp(*pkt, *pkt_size);
		// Make sure we have enough memory for the payload
		_realloc_packet(pkt, pkt_size, header_len + payload_size);
		// NK: NB!!!! Realloc can change the memory address
		icmp_header_t *icmp_packet = (icmp_header_t *)((*pkt) + header_len);
		// Copy in payload
		memcpy(&icmp_packet->payload, payload, payload_size);
	} else {
		std::ostringstream oss;
		oss << "Unsupported protocol " << std::hex << std::uppercase << layer3_protocol;
		throw PacketNotSupportedEception(oss.str());
	}
}

static uint64_t _compute_ipv4_pseudo_checksum_partial(ethernet_header_t *pkt, seth_be16_t layer4_length_be) {
	// The IPv4 pseudo-header is defined in RFC 793, Section 3.1.
	struct ipv4_pseudo_header_t {
			uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN];
			uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN];
			uint8_t zero;
			uint8_t protocol;
			seth_be16_t length;
	} __seth_packed;

	struct ipv4_pseudo_header_t pseudo_header;

	ip_header_t *ip_header = (ip_header_t *)&(pkt->payload);
	assert(ip_header->version == 4);

	ipv4_header_t *ipv4_packet = (ipv4_header_t *)ip_header;

	// Pseudo header
	memcpy(&pseudo_header.src_addr, &ipv4_packet->src_addr, sizeof(ipv4_packet->src_addr));
	memcpy(&pseudo_header.dst_addr, &ipv4_packet->dst_addr, sizeof(ipv4_packet->dst_addr));
	pseudo_header.zero = 0;
	pseudo_header.protocol = ipv4_packet->protocol;
	pseudo_header.length = layer4_length_be;
	return compute_checksum_partial((uint8_t *)&pseudo_header, sizeof(pseudo_header), 0);
}

uint16_t compute_ipv4_header_checksum(ethernet_header_t *pkt, uint16_t length) {
	is_valid_ethernet_ipv4(pkt, length);

	ipv4_header_t *ipv4_packet = (ipv4_header_t *)(pkt + SETH_PACKET_ETHERNET_HEADER_LEN);
	assert(ipv4_packet->ihl >= 5); // Minimum of 5 headers must be present

	uint16_t ip_header_size = ipv4_packet->ihl * 4; // Header size
	assert(length >= SETH_PACKET_ETHERNET_HEADER_LEN + ip_header_size);

	ipv4_header_t tmp;
	memcpy(&tmp, ipv4_packet, ip_header_size);
	tmp.checksum = 0;
	uint32_t partial_checksum = compute_checksum_partial((uint8_t *)&tmp, ip_header_size, 0);
	uint16_t checksum = compute_checksum_finalize(partial_checksum);

	return checksum;
}

uint16_t compute_ipv4_udp_checksum(ethernet_header_t *pkt, uint16_t length) {
	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IP_HEADER_LEN);
	ip_header_t *ip_header = (ip_header_t *)&(pkt->payload);
	assert(ip_header->version == 4); // Must be IPv4

	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);
	ipv4_header_t *ipv4_packet = (ipv4_header_t *)ip_header;
	assert(ipv4_packet->ihl >= 5);								  // Minimum of 5 headers must be present
	assert(ipv4_packet->protocol == SETH_PACKET_IP_PROTOCOL_UDP); // Must match UDP protocol

	uint16_t ip_header_count = ipv4_packet->ihl - 5; // Number of headers
	uint16_t ip_header_size = ipv4_packet->ihl * 4;	 // Header size

	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + ip_header_size);
	udp_header_t *udp_packet = (udp_header_t *)&(ipv4_packet->payload[ip_header_count]);

	// Grab our own UDP payload size as we obviously cannot read past the end of the buffer
	uint16_t udp_size = length - SETH_PACKET_ETHERNET_HEADER_LEN - ip_header_size;

	// Start off with the psudo header
	uint32_t partial_checksum = _compute_ipv4_pseudo_checksum_partial(pkt, seth_cpu_to_be_16(udp_size));

	// Copy packet, blank checksum, and dump payload into checksum
	udp_header_t tmp;
	memcpy(&tmp, udp_packet, SETH_PACKET_UDP_HEADER_LEN);
	tmp.checksum = 0;
	partial_checksum = compute_checksum_partial((uint8_t *)&tmp, SETH_PACKET_UDP_HEADER_LEN, partial_checksum);
	partial_checksum =
		compute_checksum_partial((uint8_t *)&udp_packet->payload, udp_size - SETH_PACKET_UDP_HEADER_LEN, partial_checksum);

	// Finally finalize the checksum
	uint16_t checksum = compute_checksum_finalize(partial_checksum);

	return checksum;
}

uint16_t compute_ipv4_icmp_checksum(ethernet_header_t *pkt, uint16_t length) {
	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IP_HEADER_LEN);
	ip_header_t *ip_header = (ip_header_t *)&(pkt->payload);
	assert(ip_header->version == 4); // Must be IPv4

	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);
	ipv4_header_t *ipv4_packet = (ipv4_header_t *)ip_header;
	assert(ipv4_packet->ihl >= 5);									// Minimum of 5 headers must be present
	assert(ipv4_packet->protocol == SETH_PACKET_IP_PROTOCOL_ICMP4); // Must match UDP protocol

	uint16_t ip_header_count = ipv4_packet->ihl - 5; // Number of headers
	uint16_t ip_header_size = ipv4_packet->ihl * 4;	 // Header size

	assert(length > SETH_PACKET_ETHERNET_HEADER_LEN + ip_header_size);
	icmp_header_t *icmp_packet = (icmp_header_t *)&(ipv4_packet->payload[ip_header_count]);

	// Grab our own UDP payload size as we obviously cannot read past the end of the buffer
	uint16_t icmp_size = length - SETH_PACKET_ETHERNET_HEADER_LEN - ip_header_size;

	// Copy packet, blank checksum, and dump payload into checksum
	icmp_header_t tmp;
	memcpy(&tmp, icmp_packet, SETH_PACKET_ICMP_HEADER_LEN);
	tmp.checksum = 0;
	uint32_t partial_checksum = compute_checksum_partial((uint8_t *)&tmp, SETH_PACKET_ICMP_HEADER_LEN, 0);
	partial_checksum =
		compute_checksum_partial((uint8_t *)&icmp_packet->payload, icmp_size - SETH_PACKET_ICMP_HEADER_LEN, partial_checksum);

	// Finally finalize the checksum
	uint16_t checksum = compute_checksum_finalize(partial_checksum);

	return checksum;
}

int is_valid_ethernet(const ethernet_header_t *packet, uint16_t size) {
	uint16_t header_len = SETH_PACKET_ETHERNET_HEADER_LEN;
	if (size < header_len) {
		std::ostringstream oss;
		oss << "Malformed ethernet packet size " << size << " < " << header_len;
		throw PacketMalformedEception(oss.str());
	}
	return header_len;
}

// This function does NOT return the header size, but the header size of the ethernet header
int is_valid_ethernet_ip(const ethernet_header_t *packet, uint16_t size) {
	uint16_t header_len = is_valid_ethernet(packet, size);

	if (size < header_len + SETH_PACKET_IP_HEADER_LEN) {
		std::ostringstream oss;
		oss << "Malformed IP packet size " << size << " < " << header_len;
		throw PacketMalformedEception(oss.str());
	}
	return header_len;
}

int is_valid_ethernet_ipv4(const ethernet_header_t *packet, uint16_t size) {
	// NB: Returns ethernet header size
	uint16_t header_len = is_valid_ethernet_ip(packet, size);

	// Minimum header size check
	if (size < header_len + SETH_PACKET_IPV4_HEADER_LEN) {
		std::ostringstream oss;
		oss << "Malformed IPv4 packet size " << size << " < " << header_len;
		throw PacketMalformedEception(oss.str());
	}

	// Grab IPv4 packet
	const ipv4_header_t *ipv4_packet = (ipv4_header_t *)(packet + header_len);
	if (ipv4_packet->ihl < 5) {
		std::ostringstream oss;
		oss << "Malformed IPv4 packet ihl " << size << " < 5";
		throw PacketMalformedEception(oss.str());
	}

	header_len += (ipv4_packet->ihl * 4);

	if (size < header_len) {
		std::ostringstream oss;
		oss << "Malformed IPv4 packet size " << size << " < " << header_len << " (ihl*4)";
		throw PacketMalformedEception(oss.str());
	}

	return header_len;
}

int is_valid_ethernet_ipv4_udp(const ethernet_header_t *packet, uint16_t size) {
	uint16_t header_len = is_valid_ethernet_ipv4(packet, size);

	const ipv4_header_t *ipv4_packet = (ipv4_header_t *)&packet->payload;
	if (ipv4_packet->protocol != SETH_PACKET_IP_PROTOCOL_UDP) {
		std::ostringstream oss;
		oss << "Malformed IPv4 UDP packet protocol " << ipv4_packet->protocol << " != " << SETH_PACKET_IP_PROTOCOL_UDP;
		throw PacketMalformedEception(oss.str());
	}

	header_len += SETH_PACKET_UDP_HEADER_LEN;

	if (size < header_len) {
		std::ostringstream oss;
		oss << "Malformed IPv4 UDP packet size " << size << " < " << header_len;
		throw PacketMalformedEception(oss.str());
	}

	return header_len;
}

int is_valid_ethernet_ipv4_icmp(const ethernet_header_t *packet, uint16_t size) {
	uint16_t header_len = is_valid_ethernet_ipv4(packet, size);

	const ipv4_header_t *ipv4_packet = (ipv4_header_t *)&packet->payload;
	if (ipv4_packet->protocol != SETH_PACKET_IP_PROTOCOL_ICMP4) {
		std::ostringstream oss;
		oss << "Malformed IPv4 ICMP packet protocol " << ipv4_packet->protocol << " != " << SETH_PACKET_IP_PROTOCOL_UDP;
		throw PacketMalformedEception(oss.str());
	}

	header_len += SETH_PACKET_ICMP_HEADER_LEN;

	if (size < header_len) {
		std::ostringstream oss;
		oss << "Malformed IPv4 ICMP packet size " << size << " < " << header_len;
		throw PacketMalformedEception(oss.str());
	}

	return header_len;
}

*/
