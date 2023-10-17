#include "libsethnetkit/EthernetPacket.hpp"
#include "libtests/framework.hpp"



TEST_CASE("Check creating ethernet packets with IPv4 ethertype", "[ethernet-packets]") {
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

	EthernetPacket packet;
	packet.setDstMac(dst_mac);
	packet.setSrcMac(src_mac);

	std::cout << packet.asText() << std::endl;

	// packet.ethernetHeader(dst_mac, src_mac, SETH_PACKET_TYPE_ETHERNET_IPV4);

	// REQUIRE(packet.getSize() == SETH_PACKET_ETHERNET_HEADER_LEN);

	// packet.printHex();

	// uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00};
	// print_hex_dump(data, sizeof(data));
	// REQUIRE(packet.compare(&data, sizeof(data)) == 0);
}

// TEST_CASE("Check creating ethernet packets with IPv6 ethertype", "[ethernet-packets]") {
// 	uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
// 	uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

// 	PacketBuffer packet;
// 	packet.ethernetHeader(dst_mac, src_mac, SETH_PACKET_TYPE_ETHERNET_IPV6);

// 	REQUIRE(packet.getSize() == SETH_PACKET_ETHERNET_HEADER_LEN);

// 	packet.printHex();

// 	uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x86, 0xdd};
// 	print_hex_dump(data, sizeof(data));
// 	REQUIRE(packet.compare(&data, sizeof(data)) == 0);
// }
