/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "icmpv6_packet.hpp"
#include "ipv6_packet.hpp"
#include "checksum.hpp"
#include <format>
#include <sstream>

void ICMPv6Packet::_clear() {
	setNextHeader(SETH_PACKET_IP_PROTOCOL_ICMP6);

	type = 0;
	code = 0;
	checksum = 0;
}

ICMPv6Packet::ICMPv6Packet() : IPv6Packet() { _clear(); }

ICMPv6Packet::ICMPv6Packet(const std::vector<uint8_t> &data) : IPv6Packet(data) {
	_clear();
	parse(data);
}

ICMPv6Packet::~ICMPv6Packet() {}

void ICMPv6Packet::clear() {
	IPv6Packet::clear();
	ICMPv6Packet::_clear();
}

void ICMPv6Packet::parse(const std::vector<uint8_t> &data) {}

uint8_t ICMPv6Packet::getType() const { return type; }
void ICMPv6Packet::setType(uint8_t newType) { type = newType; }

uint8_t ICMPv6Packet::getCode() const { return code; }
void ICMPv6Packet::setCode(uint8_t newCode) { code = newCode; }

uint16_t ICMPv6Packet::getChecksumLayer4() const {
	// Populate UDP header to add onto the layer 3 pseudo header
	icmp6_header_t header;

	header.type = type;
	header.code = code;
	header.checksum = 0;
	header.unused1 = 0;
	header.unused2 = 0;

	// Grab the layer3 checksum
	uint32_t partial_checksum = IPv6Packet::getPseudoChecksumLayer3(getLayer4Size());
	//  Add the UDP packet header
	partial_checksum = compute_checksum_partial((uint8_t *)&header, sizeof(icmp6_header_t), partial_checksum);
	// Add payload
	partial_checksum =
		compute_checksum_partial((uint8_t *)IPv6Packet::getPayloadPointer(), IPv6Packet::getPayloadSize(), partial_checksum);
	// Finalize checksum and return
	return compute_checksum_finalize(partial_checksum);
}

uint16_t ICMPv6Packet::getHeaderOffset() const { return IPv6Packet::getHeaderOffset() + IPv6Packet::getHeaderSize(); }
uint16_t ICMPv6Packet::getHeaderSize() const { return sizeof(icmp6_header_t); }
uint16_t ICMPv6Packet::getHeaderSizeTotal() const { return IPv6Packet::getHeaderSizeTotal() + getHeaderSize(); }
uint16_t ICMPv6Packet::getLayer4Size() const { return getHeaderSize() + IPv6Packet::getPayloadSize(); }

std::string ICMPv6Packet::asText() const {
	std::ostringstream oss;

	oss << IPv6Packet::asText() << std::endl;

	oss << "==> ICMPv6 Packet" << std::endl;

	oss << std::format("*Header Offset : {}", getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size   : {}", getHeaderSize()) << std::endl;

	oss << std::format("Type           : {}", getType()) << std::endl;
	oss << std::format("Code           : {}", getCode()) << std::endl;
	oss << std::format("Checksum       : {:04X}", getChecksumLayer4()) << std::endl;

	return oss.str();
}

std::string ICMPv6Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);

	oss << IPv6Packet::asBinary();

	// Build header to dump into the stream
	icmp6_header_t header;

	header.type = type;
	header.code = code;
	header.unused1 = 0;
	header.unused2 = 0;

	header.checksum = accl::cpu_to_be_16(getChecksumLayer4());

	// Write out header to stream
	oss.write(reinterpret_cast<const char *>(&header), sizeof(icmp6_header_t));
	// Write out payload to stream
	oss.write(reinterpret_cast<const char *>(getPayloadPointer()), getPayloadSize());

	return oss.str();
}
