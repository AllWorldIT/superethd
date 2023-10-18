/*
 * Packet handling.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

// FIXME
/*
extern "C" {
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdint.h>
}

#include <stdexcept>
#include <string>
#include <vector>

#include "common.hpp"
#include "endian.hpp"

#include "libsethnetkit/Packet.hpp"

#include "libsethnetkit/EthernetPacket.hpp"

#include "libsethnetkit/IPPacket.hpp"

#include "libsethnetkit/IPv4Packet.hpp"

#include "libsethnetkit/ICMPv4Packet.hpp"

#include "libsethnetkit/UDPPacket.hpp"



#define SETH_PACKET_TYPE_ETHERNET_ARP 0x0806
#define SETH_PACKET_TYPE_ETHERNET_VLAN 0x8100


#define SETH_PACKET_IP_HEADER_LEN 1



#define SETH_PACKET_ICMP_HEADER_LEN 8
#define SETH_PACKET_UDP_HEADER_LEN 8
#define SETH_PACKET_TCP_HEADER_LEN 20

#define SETH_PACKET_ICMPV4_TYPE_ECHO_REPLY 0
#define SETH_PACKET_ICMPV4_TYPE_ECHO_REQUEST 8


ethernet_header_t *add_packet_ethernet_header(ethernet_header_t **pkt, uint16_t *pkt_size, uint8_t dst_mac[ETH_ALEN],
											  uint8_t src_mac[ETH_ALEN], uint16_t ethertype);

ipv4_header_t *add_packet_ipv4_header(ethernet_header_t **pkt, uint16_t *pkt_size, uint16_t id,
									  uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN], uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN]);

udp_header_t *add_packet_udp_header(ethernet_header_t **pkt, uint16_t *pkt_size, uint16_t src_port, uint16_t dst_port);
icmp_header_t *add_packet_icmp_header(ethernet_header_t **pkt, uint16_t *pkt_size, uint8_t type, uint8_t code);
void add_packet_icmp_echo_header(ethernet_header_t **pkt, uint16_t *pkt_size, uint16_t identifier, uint16_t sequence);
void add_payload(ethernet_header_t **pkt, uint16_t *pkt_size, uint8_t *payload, uint16_t payload_size);
void set_length(ethernet_header_t **pkt, uint16_t pkt_size);
void set_checksums(ethernet_header_t **pkt, uint16_t pkt_size);

uint16_t compute_ipv4_header_checksum(ethernet_header_t *pkt, uint16_t length);
uint16_t compute_ipv4_udp_checksum(ethernet_header_t *pkt, uint16_t length);
uint16_t compute_ipv4_icmp_checksum(ethernet_header_t *pkt, uint16_t length);

int is_valid_ethernet(const ethernet_header_t *packet, uint16_t size);
int is_valid_ethernet_ip(const ethernet_header_t *packet, uint16_t size);
int is_valid_ethernet_ipv4(const ethernet_header_t *packet, uint16_t size);
int is_valid_ethernet_ipv4_udp(const ethernet_header_t *packet, uint16_t size);
int is_valid_ethernet_ipv4_icmp(const ethernet_header_t *packet, uint16_t size);
*/