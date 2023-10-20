#include "libtests/framework.hpp"

TEST_CASE("Check creating IPv4 ICMP packets", "[ethernet-ipv4-icmp]") {
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};

	ICMPv4Packet packet;
	packet.setDstMac(dst_mac);
	packet.setSrcMac(src_mac);
	packet.setDstAddr(dst_ip);
	packet.setSrcAddr(src_ip);

	packet.setType(8);
	packet.setCode(0);

	packet.printText();
	packet.printHex();

	const unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x08, 0x00,
								  0x45, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x9E, 0xC1, 0xAC, 0x10,
								  0x65, 0x66, 0xC0, 0xA8, 0x0A, 0x01, 0x08, 0x00, 0xF7, 0xFF, 0x00, 0x00, 0x00, 0x00};
	std::string bin(reinterpret_cast<const char *>(data), sizeof(data));
	REQUIRE(packet.compare(bin) == true);
}

TEST_CASE("Check creating IPv4 ICMP packets with payload", "[ethernet-ipv4-icmp]") {
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};
	SequenceDataGenerator payloadSeq = SequenceDataGenerator(100);

	std::string payloadString = payloadSeq.asString();
	std::vector<uint8_t> payloadBytes = payloadSeq.asBytes();

	ICMPv4Packet packet;
	packet.setDstMac(dst_mac);
	packet.setSrcMac(src_mac);
	packet.setDstAddr(dst_ip);
	packet.setSrcAddr(src_ip);

	packet.setType(8);
	packet.setCode(0);

	packet.addPayload(payloadBytes);

	packet.printText();
	packet.printHex();

	const unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x08, 0x00, 0x45, 0x00,
								  0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x9E, 0x5D, 0xAC, 0x10, 0x65, 0x66, 0xC0, 0xA8,
								  0x0A, 0x01, 0x08, 0x00, 0x57, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x41, 0x30, 0x31, 0x32, 0x33, 0x34,
								  0x35, 0x36, 0x37, 0x38, 0x39, 0x42, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
								  0x43, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x44, 0x30, 0x31, 0x32, 0x33,
								  0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x45, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
								  0x39, 0x46, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x47, 0x30, 0x31, 0x32,
								  0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x48, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
								  0x38, 0x39, 0x49, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x4A};
	std::string bin(reinterpret_cast<const char *>(data), sizeof(data));
	REQUIRE(packet.compare(bin) == true);
}
