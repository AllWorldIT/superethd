/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "libtests/framework.hpp"

TEST_CASE("Check creating IPv4 TCP packets", "[ethernet-ipv4-tcp]") {
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};
	accl::SequenceDataGenerator payloadSeq = accl::SequenceDataGenerator(100);

	std::string payloadString = payloadSeq.asString();
	std::vector<uint8_t> payloadBytes = payloadSeq.asBytes();

	TCPv4Packet packet;

	packet.setDstMac(dst_mac);
	packet.setSrcMac(src_mac);
	packet.setDstAddr(dst_ip);
	packet.setSrcAddr(src_ip);

	packet.setSrcPort(12345);
	packet.setDstPort(54321);
	packet.setSequence(2);
	packet.setAck(1);
	packet.setOptACK(true);

	packet.addPayload(payloadBytes);

	packet.printText();
	packet.printHex();

	const unsigned char data[] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x08, 0x00, 0x45, 0x00, 0x00, 0x8C, 0x00, 0x00,
		0x00, 0x00, 0x40, 0x06, 0x9E, 0x4C, 0xAC, 0x10, 0x65, 0x66, 0xC0, 0xA8, 0x0A, 0x01, 0x30, 0x39, 0xD4, 0x31, 0x00, 0x00,
		0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x50, 0x10, 0x00, 0x00, 0x2E, 0x42, 0x00, 0x00, 0x41, 0x30, 0x31, 0x32, 0x33, 0x34,
		0x35, 0x36, 0x37, 0x38, 0x39, 0x42, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x43, 0x30, 0x31, 0x32,
		0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x44, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x45, 0x30,
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x46, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
		0x47, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x48, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x49, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x4A};
	std::string bin(reinterpret_cast<const char *>(data), sizeof(data));
	REQUIRE(packet.compare(bin) == true);
}

TEST_CASE("Check parsing a packet ends up in the correct result", "[ethernet-ipv4-tcp][!mayfail]") {
	const unsigned char data[] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x08, 0x00, 0x45, 0x00, 0x00, 0x8C, 0x00, 0x00,
		0x00, 0x00, 0x40, 0x06, 0x9E, 0x4C, 0xAC, 0x10, 0x65, 0x66, 0xC0, 0xA8, 0x0A, 0x01, 0x30, 0x39, 0xD4, 0x31, 0x00, 0x00,
		0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x50, 0x10, 0x00, 0x00, 0x2E, 0x42, 0x00, 0x00, 0x41, 0x30, 0x31, 0x32, 0x33, 0x34,
		0x35, 0x36, 0x37, 0x38, 0x39, 0x42, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x43, 0x30, 0x31, 0x32,
		0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x44, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x45, 0x30,
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x46, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
		0x47, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x48, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x49, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x4A};

	std::vector<uint8_t> data_v(data, data + sizeof(data) / sizeof(data[0]));
	TCPv4Packet packet(data_v);

	packet.printText();
	packet.printHex();

	std::string bin(reinterpret_cast<const char *>(data), sizeof(data));
	REQUIRE(packet.compare(bin) == true);
}