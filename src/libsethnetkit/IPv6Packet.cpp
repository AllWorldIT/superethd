/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "IPv6Packet.hpp"
#include "IPPacket.hpp"
#include "checksum.hpp"

void IPv6Packet::_clear() {
	setEthertype(SETH_PACKET_ETHERTYPE_ETHERNET_IPV6);
	setVersion(SETH_PACKET_IP_VERSION_IPV6);

	priority = 0;
	flow_label = 0;
	next_header = 0;
	hop_limit = 64;
	// Clear IP's
	for (auto &element : dst_addr) {
		element = 0;
	}
	for (auto &element : src_addr) {
		element = 0;
	}
}

uint16_t IPv6Packet::_ipv6HeaderPayloadLength() const { return getLayer3Size() - IPv6Packet::getHeaderSize(); }

IPv6Packet::IPv6Packet() : IPPacket() { _clear(); }

IPv6Packet::IPv6Packet(const std::vector<uint8_t> &data) : IPPacket(data) {
	_clear();
	parse(data);
}

IPv6Packet::~IPv6Packet() {}

void IPv6Packet::clear() {
	IPPacket::clear();
	IPv6Packet::_clear();
}

void IPv6Packet::parse(const std::vector<uint8_t> &data) { setVersion(SETH_PACKET_IP_VERSION_IPV6); }

uint8_t IPv6Packet::getTrafficClass() const { return priority; }
void IPv6Packet::setTrafficClass(uint8_t newTrafficClass) { priority = newTrafficClass; }

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

uint32_t IPv6Packet::getPseudoChecksumLayer3(uint16_t length) const {

	// The IPv6 pseudo-header is defined in RFC 2460, Section 8.1.
	struct ipv6_pseudo_header_t {
			uint8_t src_addr[SETH_PACKET_IPV6_IP_LEN];
			uint8_t dst_addr[SETH_PACKET_IPV6_IP_LEN];
			seth_be32_t length;
			uint32_t zero : 24;
			uint8_t next_header; // AKA protocol
	} SETH_PACKED_ATTRIBUTES;

	ipv6_pseudo_header_t header;

	std::copy(src_addr.begin(), src_addr.end(), header.src_addr);
	std::copy(dst_addr.begin(), dst_addr.end(), header.dst_addr);

	header.length = seth_cpu_to_be_16(length);
	header.zero = 0;
	header.next_header = next_header;

	return compute_checksum_partial((uint8_t *)&header, sizeof(ipv6_pseudo_header_t), 0);
}

// TODO: include header options size
uint16_t IPv6Packet::getHeaderSize() const { return sizeof(ipv6_header_t); }
uint16_t IPv6Packet::getHeaderSizeTotal() const { return IPv6Packet::getHeaderSize(); }
uint16_t IPv6Packet::getLayer3Size() const { return getHeaderSizeTotal() + getPayloadSize(); }

std::string IPv6Packet::asText() const {
	std::ostringstream oss;

	oss << Packet::asText() << std::endl;

	oss << "==> IPv6" << std::endl;

	oss << std::format("*Header Offset  : {}", IPv6Packet::getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size    : {}", IPv6Packet::getHeaderSize()) << std::endl;

	oss << std::format("Version         : {}", getVersion()) << std::endl;
	oss << std::format("Traffic Class   : {}", getTrafficClass()) << std::endl;
	oss << std::format("Flow Label      : {}", getFlowLabel()) << std::endl;
	oss << std::format("Payload Length  : {}", _ipv6HeaderPayloadLength()) << std::endl;
	oss << std::format("Next Header     : {}", getNextHeader()) << std::endl;
	oss << std::format("Hop Limit       : {}", getHopLimit()) << std::endl;

	std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> ip;

	ip = getDstAddr();
	oss << std::format("Destination IP  : "
					   "{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}",
					   ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7], ip[8], ip[9], ip[10], ip[11], ip[12], ip[13], ip[14],
					   ip[15])
		<< std::endl;

	ip = getSrcAddr();
	oss << std::format("Source IP       : "
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

	header.version = version;
	header.priority = priority;
	header.flow_label = flow_label;
	header.next_header = next_header;
	header.hop_limit = hop_limit;
	header.payload_length = seth_cpu_to_be_16(_ipv6HeaderPayloadLength());

	std::copy(src_addr.begin(), src_addr.end(), header.src_addr);
	std::copy(dst_addr.begin(), dst_addr.end(), header.dst_addr);

	oss.write(reinterpret_cast<const char *>(&header), sizeof(ipv6_header_t));

	return oss.str();
}