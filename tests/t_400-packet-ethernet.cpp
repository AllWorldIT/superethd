#include "libtests/framework.hpp"


Test(ethernet_packets, plain_ethernet_packet_ipv4) {
	ethernet_packet_t *packet = NULL;
	uint16_t pkt_size = 0;
	uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

	add_packet_ethernet_header(&packet, &pkt_size, dst_mac, src_mac, SETH_PACKET_TYPE_ETHERNET_IPV4);
	cr_assert(eq(u16, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN), "Ensure new packet size is correct.");

	print_hex_dump((uint8_t *)packet, pkt_size);

	uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00};
	struct cr_mem mem_arr1(packet, pkt_size);
	struct cr_mem mem_arr2(&data, sizeof(data));
	cr_assert(eq(mem, mem_arr1, mem_arr2, sizeof(data)), "Ensure contents of created ethernet packet are correct.");

	free(packet);
}

Test(ethernet_packets, plain_ethernet_packet_ipv6) {
	ethernet_packet_t *packet = NULL;
	uint16_t pkt_size = 0;
	uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

	add_packet_ethernet_header(&packet, &pkt_size, dst_mac, src_mac, SETH_PACKET_TYPE_ETHERNET_IPV4);
	cr_assert(eq(u16, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN), "Ensure new packet size is correct.");

	print_hex_dump((uint8_t *)packet, pkt_size);

	uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00};
	struct cr_mem mem_arr1(packet, pkt_size);
	struct cr_mem mem_arr2(&data, sizeof(data));
	cr_assert(eq(mem, mem_arr1, mem_arr2, sizeof(data)), "Ensure contents of created ethernet packet are correct.");

	free(packet);
}
