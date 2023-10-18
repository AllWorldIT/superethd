/*
 * Packet buffer handling.
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

#include "PacketBuffer.hpp"
#include "packet.hpp"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

PacketBuffer::PacketBuffer() : data() {}

PacketBuffer::PacketBuffer(uint16_t size) : data(size) {}



bool PacketBuffer::copyData(const uint8_t *src, uint16_t len) {
	if (len > data.size()) {
		return false;
	}
	std::copy(src, src + len, data.begin());
	return true;
}






ethernet_header_t *PacketBuffer::getHeaderEthernet() {
	if (data.size() < sizeof(ethernet_header_t)) {
		std::ostringstream oss;
		oss << "Packet too small to be an ethernet packet, size " << data.size() << " vs. " << sizeof(ethernet_header_t)
			<< " (required)";
		throw PacketOperationInvalid(oss.str());
	}
	return reinterpret_cast<ethernet_header_t *>(data.data());
}
uint16_t PacketBuffer::getEthernetHeaderSize() {
	return SETH_PACKET_ETHERNET_HEADER_LEN;
}


ip_header_t *PacketBuffer::getHeaderIP() {
	auto header_offset = getEthernetHeaderSize();
	auto header_total = header_offset + sizeof(ip_header_t);

	if (data.size() < header_total) {
		std::ostringstream oss;
		oss << "Packet too small to be an IP packet, size " << data.size() << " vs. " << header_total << " (required)";
		throw PacketOperationInvalid(oss.str());
	}
	return reinterpret_cast<ip_header_t *>(data[header_offset]);
}

ipv4_header_t *PacketBuffer::getHeaderIPv4() {
	auto header_offset = getEthernetHeaderSize();
	auto header_total = header_offset + sizeof(ipv4_header_t);

	if (data.size() < header_total) {
		std::ostringstream oss;
		oss << "Packet too small to be an IPv4 packet, size " << data.size() << " vs. " << header_total << " (required)";
		throw PacketOperationInvalid(oss.str());
	}

	return reinterpret_cast<ipv4_header_t *>(data[header_offset]);
}

uint16_t PacketBuffer::getHeaderIPv4HeaderSize() {
	return getHeaderIPv4()->ihl * 4;
}

udp_header_t *PacketBuffer::getHeaderUDPv4() {
	auto header_offset = sizeof(ethernet_header_t) + getHeaderIPv4HeaderSize();
	auto header_total = header_offset + sizeof(ipv4_header_t);

	if (data.size() < header_total) {
		std::ostringstream oss;
		oss << "Packet too small to be an IPv4 packet, size " << data.size() << " vs. " << header_total << " (required)";
		throw PacketOperationInvalid(oss.str());
	}

	return reinterpret_cast<ipv4_header_t *>(data[header_offset]);
}

void PacketBuffer::ethernetHeader(uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN], uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN],
								  uint16_t ethertype) {
	// Resize buffer
	if (getSize() < SETH_PACKET_ETHERNET_HEADER_LEN) {
		resize(SETH_PACKET_ETHERNET_HEADER_LEN);
	}
	// Overlay ethernet packet...
	ethernet_header_t *ethernet_packet = getHeaderEthernet();
	// Set header values
	std::copy(dst_mac, dst_mac + SETH_PACKET_ETHERNET_MAC_LEN, ethernet_packet->dst_mac);
	std::copy(src_mac, src_mac + SETH_PACKET_ETHERNET_MAC_LEN, ethernet_packet->src_mac);
	ethernet_packet->ethertype = seth_cpu_to_be_16(ethertype);
}

void PacketBuffer::IPHeader(uint16_t id, uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN], uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN]) {
	// Resize buffer
	if (getSize() < SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IP_HEADER_LEN) {
		resize(SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IP_HEADER_LEN);
	}
	// Overlay ethernet packet...
	ethernet_header_t *ethernet_packet = getHeaderEthernet();
	ip_header_t *ip_packet = getHeaderIP();

	// Set header values
	ip_packet->version = SETH_PACKET_IP_VERSION_IPV4;
	ip_packet->ihl = 5;
	ip_packet->dscp = 0;
	ip_packet->ecn = 0;
	ip_packet->total_length = 0;

	ip_packet->id = seth_cpu_to_be_16(id - 1);

	ip_packet->frag_off = 0;

	ip_packet->ttl = 64;
	ip_packet->protocol = 0;

	ip_packet->checksum = 0;

	std::copy(src_addr, src_addr + SETH_PACKET_IPV4_IP_LEN, ip_packet->src_addr);
	std::copy(dst_addr, dst_addr + SETH_PACKET_IPV4_IP_LEN, ip_packet->dst_addr);

	// Ensure the ethertype is correctly set
	ethernet_packet->ethertype = seth_cpu_to_be_16(SETH_PACKET_TYPE_ETHERNET_IPV4);
}

void PacketBuffer::IP4Header(uint16_t id, uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN], uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN]) {
	// Resize buffer
	if (getSize() < SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN) {
		resize(SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);
	}
	// Overlay ethernet packet...
	ethernet_header_t *ethernet_packet = getHeaderEthernet();
	ipv4_header_t *ipv4_packet = reinterpret_cast<ipv4_header_t *> (data[sizeof(ethernet_header_t)]);

	// Set header values
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

	std::copy(src_addr, src_addr + SETH_PACKET_IPV4_IP_LEN, ipv4_packet->src_addr);
	std::copy(dst_addr, dst_addr + SETH_PACKET_IPV4_IP_LEN, ipv4_packet->dst_addr);

	// Ensure the ethertype is correctly set
	ethernet_packet->ethertype = seth_cpu_to_be_16(SETH_PACKET_TYPE_ETHERNET_IPV4);
}