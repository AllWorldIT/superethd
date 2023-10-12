#include "libtests/framework.hpp"


Test(ethernet_ipv4_udp_packets, ethernet_ipv4_with_udp_payload) {
	ethernet_packet_t *packet = NULL;
	uint16_t pkt_size = 0;
	uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	uint8_t dst_ip[SETH_PACKET_IPV4_IP_LEN] = {192, 168, 10, 1};
	uint8_t src_ip[SETH_PACKET_IPV4_IP_LEN] = {172, 16, 101, 102};

	add_packet_ethernet_header(&packet, &pkt_size, dst_mac, src_mac, SETH_PACKET_TYPE_ETHERNET_IPV4);
	cr_assert(eq(u16, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN), "Ensure new packet size is correct.");

	add_packet_ipv4_header(&packet, &pkt_size, 1, src_ip, dst_ip);
	cr_assert(eq(u16, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN),
			  "Ensure IPv4 packet size is correct.");

	add_packet_udp_header(&packet, &pkt_size, 12345, 54321);
	cr_assert(eq(u16, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN + SETH_PACKET_UDP_HEADER_LEN),
			  "Ensure IPv4 packet size is correct.");

	char *payload = create_sequence_data(100);
	add_payload(&packet, &pkt_size, (uint8_t *) payload, 100);
	print_hex_dump((uint8_t *)payload, 100);

	// Set up packet length
	set_length(&packet, pkt_size);
	// Set up checksums
	set_checksums(&packet, pkt_size);

	FPRINTF("PACKET PAYLOAD TEST: pkt_size=%u", pkt_size);
	print_hex_dump((uint8_t *)packet, pkt_size);

	uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00,
					  0x45, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x9e, 0xb1, 0xac, 0x10,
					  0x65, 0x66, 0xc0, 0xa8, 0x0a, 0x01, 0x30, 0x39, 0xd4, 0x31, 0x00, 0x08, 0x1f, 0x53};
	struct cr_mem mem_arr1(packet, pkt_size);
	struct cr_mem mem_arr2(&data, sizeof(data));
	cr_assert(eq(mem_arr1, mem_arr2), "Ensure contents of created ethernet packet are correct.");

	free(payload);
	free(packet);
}
