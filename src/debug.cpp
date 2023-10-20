/*
 * Debug functionality.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "debug.hpp"

extern "C" {
#include <linux/if_ether.h>
#include <linux/udp.h>
#include <net/ethernet.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include "endian.hpp"
#include "packet.hpp"
#include "util.hpp"

void print_hex_dump(const uint8_t *buffer, size_t length) {
	for (size_t i = 0; i < length; i += 16) {
		// Print the offset
		printf("%04zx  ", i);

		// Print the hex representation of the next 16 bytes (or fewer if near the end)
		for (size_t j = 0; j < 16; j++) {
			if (i + j < length) {
				printf("%02x ", buffer[i + j]);
			} else {
				printf("   ");	// Print spaces for any bytes beyond the buffer length
			}
		}

		printf("\n");
	}
}

void print_in_bits(const uint8_t *buffer, size_t length) {
	for (size_t i = 0; i < length; i++) {
		for (int j = 7; j >= 0; j--) {			 // Iterate over each bit in byte
			printf("%d", (buffer[i] >> j) & 1);	 // Shift and mask to get the bit
		}

		// Print a space for byte separation (optional)
		printf(" ");

		// Introduce a line break after every 8 bytes (64 bits)
		if ((i + 1) % 8 == 0) {
			printf("\n");
		}
	}
}


// FIXME

// void print_ipv4_header(const ethernet_header_t *packet, uint16_t size) {
// 	if (!is_valid_ethernet_ipv4(packet, size)) return;

// 	const ipv4_header_t *ipv4_packet = (ipv4_header_t *)&packet->payload;

// 	uint16_t checksum = compute_ipv4_header_checksum((ethernet_header_t *)packet, size);

// 	fprintf(stderr, "==> IPv4\n");
// 	fprintf(stderr, "IP Header Len..: %u\n", ipv4_packet->ihl * 4);
// 	fprintf(stderr, "DSCP...........: %u\n", ipv4_packet->dscp);
// 	fprintf(stderr, "ECN............: %u\n", ipv4_packet->ecn);
// 	fprintf(stderr, "Total Length...: %u\n", seth_be_to_cpu_16(ipv4_packet->total_length));
// 	fprintf(stderr, "ID.............: %u\n", seth_be_to_cpu_16(ipv4_packet->id) + 1);
// 	fprintf(stderr, "Frag Offset....: %u\n", seth_be_to_cpu_16(ipv4_packet->frag_off));
// 	fprintf(stderr, "TTL............: %u\n", ipv4_packet->ttl);
// 	fprintf(stderr, "Protocol.......: %u\n", ipv4_packet->protocol);
// 	fprintf(stderr, "Checksum.......: 0x%04X [check: 0x%04X]\n", seth_be_to_cpu_16(ipv4_packet->checksum), checksum);
// 	fprintf(stderr, "Source IP......: %u.%u.%u.%u\n", ipv4_packet->src_addr[0], ipv4_packet->src_addr[1], ipv4_packet->src_addr[2],
// 			ipv4_packet->src_addr[3]);
// 	fprintf(stderr, "Destination IP.: %u.%u.%u.%u\n", ipv4_packet->dst_addr[0], ipv4_packet->dst_addr[1], ipv4_packet->dst_addr[2],
// 			ipv4_packet->dst_addr[3]);

// 	if (ipv4_packet->protocol == SETH_PACKET_IP_PROTOCOL_ICMP4) {
// 		fprintf(stderr, "\n");
// 		print_icmp_header(packet, size);
// 	} else if (ipv4_packet->protocol == SETH_PACKET_IP_PROTOCOL_UDP) {
// 		fprintf(stderr, "\n");
// 		print_udp_header(packet, size);
// 	} else {
// 		DEBUG_PRINT("[DONT KNOW IP PROTOCOL: %u]", ipv4_packet->protocol);
// 		return;
// 	}
// }

// void print_udp_header(const ethernet_header_t *packet, uint16_t size) {
// 	if (!is_valid_ethernet_ipv4_udp(packet, size)) return;

// 	// Check full IPv4 packet size
// 	const ipv4_header_t *ipv4_packet = (ipv4_header_t *)(&packet->payload);

// 	const udp_header_t *udp_packet = (udp_header_t *)((&ipv4_packet->payload[ipv4_packet->ihl - 5]));
// 	uint16_t checksum = compute_ipv4_udp_checksum((ethernet_header_t *)packet, size);

// 	fprintf(stderr, "==> UDP\n");
// 	fprintf(stderr, "IP Header Len..: %u\n", ipv4_packet->ihl * 4);
// 	fprintf(stderr, "Source Port....: %u\n", seth_be_to_cpu_16(udp_packet->src_port));
// 	fprintf(stderr, "Dest. Port.....: %u\n", seth_be_to_cpu_16(udp_packet->dst_port));
// 	fprintf(stderr, "Length.........: %u\n", seth_be_to_cpu_16(udp_packet->length));
// 	fprintf(stderr, "Checksum.......: 0x%04X [calc: 0x%04X]\n", seth_be_to_cpu_16(udp_packet->checksum), checksum);
// }

// void print_icmp_header(const ethernet_header_t *packet, uint16_t size) {
// 	if (!is_valid_ethernet_ipv4_icmp(packet, size)) return;

// 	// Check full IPv4 packet size
// 	const ipv4_header_t *ipv4_packet = (ipv4_header_t *)(&packet->payload);

// 	const icmp_header_t *icmp_packet = (icmp_header_t *)((&ipv4_packet->payload[ipv4_packet->ihl - 5]));
// 	uint16_t checksum = compute_ipv4_icmp_checksum((ethernet_header_t *)packet, size);

// 	fprintf(stderr, "==> ICMP\n");
// 	fprintf(stderr, "IP Header Len..: %u\n", ipv4_packet->ihl * 4);
// 	fprintf(stderr, "Type...........: %u\n", icmp_packet->type);
// 	fprintf(stderr, "Code...........: %u\n", icmp_packet->code);
// 	fprintf(stderr, "Checksum.......: 0x%04X [calc: 0x%04X]\n", seth_be_to_cpu_16(icmp_packet->checksum), checksum);
// }