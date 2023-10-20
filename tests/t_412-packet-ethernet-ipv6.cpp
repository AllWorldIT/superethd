/*
 * IPv6 packet testing.
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

#include "libtests/framework.hpp"

TEST_CASE("Check creating IPv6 packets used with IPv6 ethertype", "[ethernet-ipv6]") {

	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> dst_ip = {0xfe, 0xc0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
														   0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e};
	std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> src_ip = {0xfe, 0xc0, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60,
														   0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0};

	IPv6Packet packet;
	packet.setDstMac(dst_mac);
	packet.setSrcMac(src_mac);
	packet.setDstAddr(dst_ip);
	packet.setSrcAddr(src_ip);

	packet.setNextHeader(0xfe); // Experimentation and testing protocol

	packet.printText();
	packet.printHex();

	const unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x86, 0xDD,
								  0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x40, 0xFE, 0xC0, 0x10, 0x20, 0x30, 0x40,
								  0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xFE, 0xC0, 0x01, 0x02,
								  0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E};
	std::string bin(reinterpret_cast<const char *>(data), sizeof(data));
	REQUIRE(packet.compare(bin) == true);
}
