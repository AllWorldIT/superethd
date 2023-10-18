#include "libtests/framework.hpp"

TEST_CASE("Check creating IPv4 UDP packets", "[ethernet-ipv4-udp]") {
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};
	SequenceDataGenerator payloadSeq = SequenceDataGenerator(1000);

	std::string payloadString = payloadSeq.asString();
	std::vector<uint8_t> payloadBytes = payloadSeq.asBytes();

	UDPv4Packet packet;
	packet.setDstMac(dst_mac);
	packet.setSrcMac(src_mac);
	packet.setDstAddr(dst_ip);
	packet.setSrcAddr(src_ip);
	packet.addPayload(payloadBytes);

	std::cout << "Payload size created: " << payloadString.size() << std::endl;
	std::cout << "Payload: " << payloadString << std::endl;

	std::cout << packet.asText() << std::endl;

	// add_packet_ethernet_header(&packet, &pkt_size, dst_mac, src_mac, SETH_PACKET_TYPE_ETHERNET_IPV4);
	// REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN);

	// add_packet_ipv4_header(&packet, &pkt_size, 1, src_ip, dst_ip);
	// REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);

	// add_packet_udp_header(&packet, &pkt_size, 12345, 54321);
	// REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN + SETH_PACKET_UDP_HEADER_LEN);

	// // Set up packet length
	// set_length(&packet, pkt_size);
	// // Set up checksums
	// set_checksums(&packet, pkt_size);

	// print_hex_dump((uint8_t *)packet, pkt_size);

	// uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00,
	// 				  0x45, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x9e, 0xb1, 0xac, 0x10,
	// 				  0x65, 0x66, 0xc0, 0xa8, 0x0a, 0x01, 0x30, 0x39, 0xd4, 0x31, 0x00, 0x08, 0x1f, 0x53};
	// print_hex_dump((uint8_t *)&data, sizeof(data));
	// REQUIRE(memcmp(packet, &data, sizeof(data)) == 0);

	// free(packet);
}

// TEST_CASE("Check creating IPv4 UDP packets with payload", "ethernet-ipv4-udp") {
// 	ethernet_header_t *packet = NULL;
// 	uint16_t pkt_size = 0;
// 	uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
// 	uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
// 	uint8_t dst_ip[SETH_PACKET_IPV4_IP_LEN] = {192, 168, 10, 1};
// 	uint8_t src_ip[SETH_PACKET_IPV4_IP_LEN] = {172, 16, 101, 102};

// 	add_packet_ethernet_header(&packet, &pkt_size, dst_mac, src_mac, SETH_PACKET_TYPE_ETHERNET_IPV4);
// 	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN);

// 	add_packet_ipv4_header(&packet, &pkt_size, 1, src_ip, dst_ip);
// 	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);

// 	add_packet_udp_header(&packet, &pkt_size, 12345, 54321);
// 	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN + SETH_PACKET_UDP_HEADER_LEN);

// 	char *payload = create_sequence_data(100);
// 	add_payload(&packet, &pkt_size, (uint8_t *)payload, 100);
// 	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN + SETH_PACKET_UDP_HEADER_LEN + 100);

// 	print_hex_dump((uint8_t *)payload, 100);

// 	// Set up packet length
// 	set_length(&packet, pkt_size);
// 	// Set up checksums
// 	set_checksums(&packet, pkt_size);

// 	print_hex_dump((uint8_t *)packet, pkt_size);
// 	print_ethernet_frame(packet, pkt_size);

// 	uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00, 0x45, 0x00, 0x00, 0x80,
// 					  0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x9e, 0x4d, 0xac, 0x10, 0x65, 0x66, 0xc0, 0xa8, 0x0a, 0x01, 0x30, 0x39,
// 					  0xd4, 0x31, 0x00, 0x6c, 0x7d, 0xea, 0x41, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x42,
// 					  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x43, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
// 					  0x37, 0x38, 0x39, 0x44, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x45, 0x30, 0x31, 0x32,
// 					  0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x46, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
// 					  0x47, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x48, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
// 					  0x36, 0x37, 0x38, 0x39, 0x49, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x4a};
// 	print_hex_dump((uint8_t *)&data, sizeof(data));
// 	REQUIRE(memcmp(packet, &data, sizeof(data)) == 0);

// 	free(payload);
// 	free(packet);
// }
