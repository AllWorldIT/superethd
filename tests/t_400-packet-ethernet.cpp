/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "libsethnetkit/EthernetPacket.hpp"
#include "libsethnetkit/IPv4Packet.hpp"
#include "libtests/framework.hpp"

TEST_CASE("Check creating ethernet packets with IPv4 ethertype", "[ethernet-packets]") {
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

	EthernetPacket packet;
	packet.setDstMac(dst_mac);
	packet.setSrcMac(src_mac);
	packet.setEthertype(SETH_PACKET_ETHERTYPE_ETHERNET_IPV4);

	packet.printText();
	packet.printHex();

	const char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00};
	std::string bin(reinterpret_cast<const char *>(data), sizeof(data));
	REQUIRE(packet.compare(bin) == true);
}

TEST_CASE("Check creating ethernet packets with IPv6 ethertype", "[ethernet-packets]") {
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

	EthernetPacket packet;
	packet.setDstMac(dst_mac);
	packet.setSrcMac(src_mac);
	packet.setEthertype(SETH_PACKET_ETHERTYPE_ETHERNET_IPV6);

	packet.printText();
	packet.printHex();

	const unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x86, 0xdd};
	std::string bin(reinterpret_cast<const char *>(data), sizeof(data));
	REQUIRE(packet.compare(bin) == true);
}

TEST_CASE("Check parsing a packet ends up in the correct result", "[ethernet-packets][!mayfail]") {
	const unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x86, 0xdd};

	std::vector<uint8_t> data_v(data, data + sizeof(data) / sizeof(data[0]));
	EthernetPacket packet(data_v);

	packet.printText();
	packet.printHex();

	std::string bin(reinterpret_cast<const char *>(data), sizeof(data));
	REQUIRE(packet.compare(bin) == true);
}