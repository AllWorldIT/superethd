/*
 * Ethernet packet testing.
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
