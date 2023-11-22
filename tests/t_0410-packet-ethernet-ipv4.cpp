/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "libsethnetkit/EthernetPacket.hpp"
#include "libtests/framework.hpp"

TEST_CASE("Check creating IPv4 packets used with IPv4 ethertype", "[ethernet-ipv4]") {

	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};

	IPv4Packet packet;
	packet.setDstMac(dst_mac);
	packet.setSrcMac(src_mac);
	packet.setDstAddr(dst_ip);
	packet.setSrcAddr(src_ip);

	packet.printText();
	packet.printHex();

	const unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
								  0x08, 0x00, 0x45, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00,
								  0x9e, 0xca, 0xac, 0x10, 0x65, 0x66, 0xc0, 0xa8, 0x0a, 0x01};
	std::string bin(reinterpret_cast<const char *>(data), sizeof(data));
	REQUIRE(packet.compare(bin) == true);
}

// TODO: Not implemented yet
TEST_CASE("Check parsing a packet ends up in the correct result", "[ethernet-ipv4][!mayfail]") {
	const unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
								  0x08, 0x00, 0x45, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00,
								  0x9e, 0xca, 0xac, 0x10, 0x65, 0x66, 0xc0, 0xa8, 0x0a, 0x01};

	std::vector<uint8_t> data_v(data, data + sizeof(data) / sizeof(data[0]));
	IPv4Packet packet(data_v);

	packet.printText();
	packet.printHex();

	std::string bin((char *)data, sizeof(data));
	REQUIRE(packet.compare(bin) == true);
}

TEST_CASE("Check clearing a packet works", "[ethernet-ipv4]") {

	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};

	IPv4Packet packet;
	packet.setDstMac(dst_mac);
	packet.setSrcMac(src_mac);
	packet.setDstAddr(dst_ip);
	packet.setSrcAddr(src_ip);

	packet.printText();
	packet.printHex();

	packet.clear();

	REQUIRE(packet.getDstMac() == std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN>{0,0,0,0,0,0});
	REQUIRE(packet.getSrcMac() == std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN>{0, 0, 0, 0, 0, 0});
	REQUIRE(packet.getDstAddr() == std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN>{0, 0, 0, 0});
	REQUIRE(packet.getSrcAddr() == std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN>{0, 0, 0, 0});
}