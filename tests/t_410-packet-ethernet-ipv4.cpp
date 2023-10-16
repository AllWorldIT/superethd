#include "libtests/framework.hpp"

TEST_CASE("Check creating IPv4 packets used with IPv4 ethertype", "[ethernet-ipv4]") {
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

	// Set up packet length
	set_length(&packet, pkt_size);
	// Set up checksums
	set_checksums(&packet, pkt_size);

	print_hex_dump((uint8_t *)packet, pkt_size);

	uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00, 0x45, 0x00, 0x00,
					  0x14, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x9e, 0xca, 0xac, 0x10, 0x65, 0x66, 0xc0, 0xa8, 0x0a, 0x01};
	print_hex_dump((uint8_t *)&data, sizeof(data));
	REQUIRE(memcmp(packet, &data, sizeof(data)) == 0);

	free(packet);
}

// // The ethertype should be overwritten
// TEST_CASE("Check creating IPv4 packets used with IPv6 ethertype", "[ethernet-ipv4]") {
// 	ethernet_packet_t *packet = NULL;
// 	uint16_t pkt_size = 0;
// 	uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
// 	uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
// 	uint8_t dst_ip[SETH_PACKET_IPV4_IP_LEN] = {192, 168, 10, 1};
// 	uint8_t src_ip[SETH_PACKET_IPV4_IP_LEN] = {172, 16, 101, 102};

// 	add_packet_ethernet_header(&packet, &pkt_size, dst_mac, src_mac, SETH_PACKET_TYPE_ETHERNET_IPV6);
// 	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN);

// 	add_packet_ipv4_header(&packet, &pkt_size, 1, src_ip, dst_ip);
// 	REQUIRE(pkt_size == SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN);

// 	// Set up packet length
// 	set_length(&packet, pkt_size);
// 	// Set up checksums
// 	set_checksums(&packet, pkt_size);

// 	print_hex_dump((uint8_t *)packet, pkt_size);

// 	uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00, 0x45, 0x00, 0x00,
// 					  0x14, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x9e, 0xca, 0xac, 0x10, 0x65, 0x66, 0xc0, 0xa8, 0x0a, 0x01};
// 	REQUIRE(memcmp(packet, &data, sizeof(data)) == 0);

// 	free(packet);
// }
