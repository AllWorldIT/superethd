#include <criterion/criterion.h>
#include <criterion/new/assert.h>

#include "debug.h"
#include "libtests/framework.h"
#include "packet.h"

Test(ethernet_ipv4_icmp_packets, ethernet_ipv4_with_icmp_echo) {
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

	add_packet_icmp_echo_header(&packet, &pkt_size, 0x1234, 0x4321);
	cr_assert(eq(u16, pkt_size, SETH_PACKET_ETHERNET_HEADER_LEN + SETH_PACKET_IPV4_HEADER_LEN + SETH_PACKET_ICMP_ECHO_HEADER_LEN),
			  "Ensure IPv4 packet size is correct.");

	// Set up packet length
	set_length(&packet, pkt_size);
	// Set up checksums
	set_checksums(&packet, pkt_size);

	print_hex_dump((uint8_t *)packet, pkt_size);
	print_ethernet_frame(packet, pkt_size);

	uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x08, 0x00,
					  0x45, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x9e, 0xc1, 0xac, 0x10,
					  0x65, 0x66, 0xc0, 0xa8, 0x0a, 0x01, 0x08, 0x00, 0xa2, 0xaa, 0x12, 0x34, 0x43, 0x21};
	struct cr_mem mem_arr1 = {.data = packet, .size = pkt_size};
	struct cr_mem mem_arr2 = {.data = &data, .size = sizeof(data)};
	cr_assert(eq(mem, mem_arr1, mem_arr2, sizeof(data)), "Ensure contents of created ethernet packet are correct.");

	free(packet);
}