/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "EthernetPacket.hpp"
#include <format>
#include <ostream>
#include <sstream>

void EthernetPacket::_clear() {
	// Clear macs
	for (auto &element : dst_mac) {
		element = 0;
	}
	for (auto &element : src_mac) {
		element = 0;
	}
	// Clear ethertype
	ethertype = 0;
}

EthernetPacket::EthernetPacket() : Packet() { _clear(); }

EthernetPacket::EthernetPacket(const std::vector<uint8_t> &data) : Packet(data) {
	_clear();
	parse(data);
}

EthernetPacket::~EthernetPacket() {}

void EthernetPacket::clear() {
	Packet::clear();
	EthernetPacket::_clear();
}

void EthernetPacket::parse(const std::vector<uint8_t> &data) {}

std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> EthernetPacket::getDstMac() const { return dst_mac; }
void EthernetPacket::setDstMac(const std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> &newDstMac) { dst_mac = newDstMac; }

std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> EthernetPacket::getSrcMac() const { return src_mac; }
void EthernetPacket::setSrcMac(const std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> &newDstMac) { src_mac = newDstMac; }

uint16_t EthernetPacket::getEthertype() const { return accl::be_to_cpu_16(ethertype); }
void EthernetPacket::setEthertype(const uint16_t newEthertype) { ethertype = accl::cpu_to_be_16(newEthertype); }

uint16_t EthernetPacket::getHeaderOffset() const { return 0; }
uint16_t EthernetPacket::getHeaderSize() const { return sizeof(ethernet_header_t); }

std::string EthernetPacket::asText() const {
	std::ostringstream oss;

	oss << Packet::asText() << std::endl;

	oss << "==> Ethernet" << std::endl;

	oss << std::format("*Header Offset : {}", EthernetPacket::getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size   : {}", EthernetPacket::getHeaderSize()) << std::endl;

	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> mac;

	mac = getDstMac();
	oss << std::format("Destination MAC : {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", mac[0], mac[1], mac[2], mac[3], mac[4],
					   mac[5])
		<< std::endl;

	mac = getSrcMac();
	oss << std::format("Source MAC      : {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", mac[0], mac[1], mac[2], mac[3], mac[4],
					   mac[5])
		<< std::endl;

	oss << std::format("Ethernet Type   : 0x{:04X}", getEthertype()) << std::endl;

	return oss.str();
}

std::string EthernetPacket::asBinary() const {
	std::ostringstream oss(std::ios::binary);

	oss << Packet::asBinary();

	ethernet_header_t header;
	std::copy(dst_mac.begin(), dst_mac.end(), header.dst_mac);
	std::copy(src_mac.begin(), src_mac.end(), header.src_mac);
	header.ethertype = ethertype;

	oss.write(reinterpret_cast<const char *>(&header), sizeof(ethernet_header_t));

	return oss.str();
}