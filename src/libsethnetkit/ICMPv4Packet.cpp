/*
 * ICMPv4 packet handling.
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

#include "ICMPv4Packet.hpp"
#include "IPPacket.hpp"
#include "IPv4Packet.hpp"
#include "checksum.hpp"

void ICMPv4Packet::_clear() {
	setProtocol(SETH_PACKET_IP_PROTOCOL_ICMP4);

	type = 0;
	code = 0;
	checksum = 0;
}

ICMPv4Packet::ICMPv4Packet() : IPv4Packet() { _clear(); }

ICMPv4Packet::ICMPv4Packet(const std::vector<uint8_t> &data) : IPv4Packet(data) {}

ICMPv4Packet::~ICMPv4Packet() {}

void ICMPv4Packet::clear() {
	IPv4Packet::clear();
	ICMPv4Packet::_clear();
}

void ICMPv4Packet::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	IPv4Packet::parse(data);
}

uint8_t ICMPv4Packet::getType() const { return type; }
void ICMPv4Packet::setType(uint8_t newType) { type = newType; }

uint8_t ICMPv4Packet::getCode() const { return code; }
void ICMPv4Packet::setCode(uint8_t newCode) { code = newCode; }

uint16_t ICMPv4Packet::getChecksumLayer4() const {
	// Populate UDP header to add onto the layer 3 pseudo header
	icmp_header_t header;

	header.type = type;
	header.code = code;
	header.checksum = 0;
	header.unused = 0;

	uint32_t partial_checksum = 0;
	// Add the UDP packet header
	partial_checksum = compute_checksum_partial((uint8_t *)&header, sizeof(icmp_header_t), partial_checksum);
	// Add payload
	partial_checksum =
		compute_checksum_partial((uint8_t *)IPv4Packet::getPayloadPointer(), IPv4Packet::getPayloadSize(), partial_checksum);
	// Finalize checksum and return
	return compute_checksum_finalize(partial_checksum);
}

uint16_t ICMPv4Packet::getHeaderOffset() const { return IPv4Packet::getHeaderOffset() + IPv4Packet::getHeaderSize(); }
uint16_t ICMPv4Packet::getHeaderSize() const { return sizeof(icmp_header_t); }
uint16_t ICMPv4Packet::getHeaderSizeTotal() const { return IPv4Packet::getHeaderSizeTotal() + getHeaderSize(); }
uint16_t ICMPv4Packet::getLayer4Size() const { return getHeaderSize() + ICMPv4Packet::getPayloadSize(); }

std::string ICMPv4Packet::asText() const {
	std::ostringstream oss;

	oss << IPv4Packet::asText() << std::endl;

	oss << "==> ICMPv4 Packet" << std::endl;

	oss << std::format("*Header Offset : {}", getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size   : {}", getHeaderSize()) << std::endl;

	oss << std::format("Type           : {}", getType()) << std::endl;
	oss << std::format("Code           : {}", getCode()) << std::endl;
	oss << std::format("Checksum       : {:04X}", getChecksumLayer4()) << std::endl;

	return oss.str();
}

std::string ICMPv4Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);

	oss << IPv4Packet::asBinary();

	// Build header to dump into the stream
	icmp_header_t header;

	header.type = type;
	header.code = code;
	header.unused = 0;

	header.checksum = seth_cpu_to_be_16(getChecksumLayer4());

	// Write out header to stream
	oss.write(reinterpret_cast<const char *>(&header), sizeof(icmp_header_t));
	// Write out payload to stream
	oss.write(reinterpret_cast<const char *>(getPayloadPointer()), getPayloadSize());

	return oss.str();
}
