/*
 * IPv6 packet handling.
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

#include "IPv6Packet.hpp"
#include "IPPacket.hpp"

void IPv6Packet::_clear() {
	setVersion(SETH_PACKET_IP_VERSION_IPV6);

	traffic_class = 0;
	flow_label = 0;
	//	payload_length = 0;
	next_header = 0;
	hop_limit = 0;
	// Clear IP's
	for (auto &element : dst_addr) {
		element = 0;
	}
	for (auto &element : src_addr) {
		element = 0;
	}
}

IPv6Packet::IPv6Packet() : IPPacket() { _clear(); }

IPv6Packet::IPv6Packet(const std::vector<uint8_t> &data) : IPPacket(data) { _clear(); }

IPv6Packet::~IPv6Packet() { DEBUG_PRINT("Destruction!"); }

void IPv6Packet::clear() {
	IPPacket::clear();
	_clear();
}

void IPv6Packet::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	IPPacket::parse(data);
	setVersion(SETH_PACKET_IP_VERSION_IPV6);
}

uint8_t IPv6Packet::getTrafficClass() const { return traffic_class; }
void IPv6Packet::setTrafficClass(uint8_t newTrafficClass) { traffic_class = newTrafficClass; }

uint8_t IPv6Packet::getFlowLabel() const { return flow_label; }
void IPv6Packet::setFlowLabel(uint8_t newFlowLabel) { flow_label = newFlowLabel; }

// seth_be16_t IPv6Packet::getPayloadLength() const { return seth_be_to_cpu_16(payload_length); }

uint8_t IPv6Packet::getNextHeader() const { return next_header; }
void IPv6Packet::setNextHeader(uint8_t newNextHeader) { next_header = newNextHeader; }

uint8_t IPv6Packet::getHopLimit() const { return hop_limit; }
void IPv6Packet::setHopLimit(uint8_t newHopLimit) { hop_limit = newHopLimit; }

std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> IPv6Packet::getDstAddr() const { return dst_addr; }
void IPv6Packet::setDstAddr(std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> newDstAddr) { dst_addr = newDstAddr; }

std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> IPv6Packet::getSrcAddr() const { return src_addr; }
void IPv6Packet::setSrcAddr(std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> newSrcAddr) { src_addr = newSrcAddr; }

uint16_t IPv6Packet::getHeaderOffset() const { return IPPacket::getHeaderOffset(); }
uint16_t IPv6Packet::getHeaderSize() const {
	// TODO: include header options size
	return sizeof(ipv6_header_t);
}

std::string IPv6Packet::asText() const {
	std::ostringstream oss;

	oss << Packet::asText() << std::endl;

	oss << "==> IPv6" << std::endl;

	oss << std::format("*Header Offset  : {}", getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size    : {}", getHeaderSize()) << std::endl;

	oss << std::format("Traffic Class  : ", getTrafficClass()) << std::endl;
	oss << std::format("Flow Label     : ", getFlowLabel()) << std::endl;
	oss << std::format("Payload Length : ", 0) << std::endl;
	oss << std::format("Next Header    : ", getNextHeader()) << std::endl;
	oss << std::format("Hop Limit      : ", getHopLimit()) << std::endl;

	std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> ip;

	ip = getDstAddr();
	oss << std::format("Destination IP: "
					   "{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}",
					   ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7], ip[8], ip[9], ip[10], ip[11], ip[12], ip[13], ip[14],
					   ip[15])
		<< std::endl;

	ip = getSrcAddr();
	oss << std::format("Source IP     : "
					   "{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}",
					   ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7], ip[8], ip[9], ip[10], ip[11], ip[12], ip[13], ip[14],
					   ip[15])
		<< std::endl;

	return oss.str();
}

std::string IPv6Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);

	oss << IPPacket::asBinary();

	ipv6_header_t header;

	header.traffic_class = traffic_class;
	header.flow_label = flow_label;
	header.next_header = next_header;
	header.hop_limit = hop_limit;
	//	header.payload_length = getPayloadLength();

	std::copy(src_addr.begin(), src_addr.end(), header.src_addr);
	std::copy(dst_addr.begin(), dst_addr.end(), header.dst_addr);

	oss.write(reinterpret_cast<const char *>(&header), sizeof(ipv6_header_t));

	return oss.str();
}