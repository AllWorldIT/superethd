#include "libtests/framework.hpp"


TEST_CASE("Check creating IPv4 ICMP packets", "[ethernet-ipv4-icmp]") {
	ethernet_header_t *packet = NULL;
	uint16_t pkt_size = 0;
	uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	uint8_t dst_ip[SETH_PACKET_IPV4_IP_LEN] = {192, 168, 10, 1};
	uint8_t src_ip[SETH_PACKET_IPV4_IP_LEN] = {172, 16, 101, 102};

	add_packet_ethernet_header(&packet, &pkt_size, dst_mac, src_mac, SETH_PACKET_TYPE_ETHERNET_IPV4);
	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN);

	add_packet_ipv4_header(&packet, &pkt_size, 1, src_ip, dst_ip);
	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);

	add_packet_icmp_echo_header(&packet, &pkt_size, 0x1234, 0x4321);
	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN + SETH_PACKET_ICMP_HEADER_LEN);

	// Set up packet length
	set_length(&packet, pkt_size);
	// Set up checksums
	set_checksums(&packet, pkt_size);

	print_hex_dump((uint8_t *)packet, pkt_size);
	print_ethernet_frame(packet, pkt_size);

	uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00,
					  0x45, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x9e, 0xc1, 0xac, 0x10,
					  0x65, 0x66, 0xc0, 0xa8, 0x0a, 0x01, 0x08, 0x00, 0xa2, 0xaa, 0x12, 0x34, 0x43, 0x21};
	print_hex_dump((uint8_t *)&data, sizeof(data));
	REQUIRE(memcmp(packet, &data, sizeof(data)) == 0);

	free(packet);
}

TEST_CASE("Check creating IPv4 ICMP packets with a payload", "[ethernet-ipv4-icmp]") {
	ethernet_header_t *packet = NULL;
	uint16_t pkt_size = 0;
	uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	uint8_t dst_ip[SETH_PACKET_IPV4_IP_LEN] = {192, 168, 10, 1};
	uint8_t src_ip[SETH_PACKET_IPV4_IP_LEN] = {172, 16, 101, 102};

	add_packet_ethernet_header(&packet, &pkt_size, dst_mac, src_mac, SETH_PACKET_TYPE_ETHERNET_IPV4);
	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN);

	add_packet_ipv4_header(&packet, &pkt_size, 1, src_ip, dst_ip);
	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);

	add_packet_icmp_echo_header(&packet, &pkt_size, 0x1234, 0x4321);
	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN + SETH_PACKET_ICMP_HEADER_LEN);

	char *payload = create_sequence_data(100);
	add_payload(&packet, &pkt_size, (uint8_t *)payload, 100);
	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN + SETH_PACKET_ICMP_HEADER_LEN + 100);

	// print_hex_dump((uint8_t *)payload, 100);

	// // Set up packet length
	// set_length(&packet, pkt_size);
	// // Set up checksums
	// set_checksums(&packet, pkt_size);

	// print_hex_dump((uint8_t *)packet, pkt_size);
	// print_ethernet_frame(packet, pkt_size);

	// uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00,
	// 				  0x45, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x9e, 0xc1, 0xac, 0x10,
	// 				  0x65, 0x66, 0xc0, 0xa8, 0x0a, 0x01, 0x08, 0x00, 0xa2, 0xaa, 0x12, 0x34, 0x43, 0x21};
	// print_hex_dump((uint8_t *)&data, sizeof(data));
	// REQUIRE(memcmp(packet, &data, sizeof(data)) == 0);

	free(payload);
	free(packet);
}
