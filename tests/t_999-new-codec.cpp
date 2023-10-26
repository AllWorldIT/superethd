/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "libaccl/BufferPool.hpp"
#include "libtests/framework.hpp"

TEST_CASE("Check encoding of a packet that will fit into MSS", "[codec]") {
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};
	accl::SequenceDataGenerator payloadSeq = accl::SequenceDataGenerator(100);

	std::string payloadString = payloadSeq.asString();
	std::vector<uint8_t> payloadBytes = payloadSeq.asBytes();

	UDPv4Packet packet;

	packet.setDstMac(dst_mac);
	packet.setSrcMac(src_mac);
	packet.setDstAddr(dst_ip);
	packet.setSrcAddr(src_ip);

	packet.setSrcPort(12345);
	packet.setDstPort(54321);

	packet.addPayload(payloadBytes);

	packet.printText();
	packet.printHex();

	/*
	 * Test encoding
	 */

	// Lets fire up the encoder
	uint16_t mtu= {1500};
	uint16_t mss = mtu - 20 - 8; // IPv6 is 40
	accl::BufferPool avail_buffer_pool(mss, 10);
	accl::BufferPool enc_buffer_pool(mss);

	std::string packet_bin = packet.asBinary();

	PacketEncoder encoder(mss, mtu, &avail_buffer_pool, &enc_buffer_pool);
	encoder.encode(reinterpret_cast<char *>(packet_bin.data()), packet_bin.length());

	// Make sure we now have a packet in the enc_buffer_pool
	assert(enc_buffer_pool.getBufferCount() == 1);
	// We should have 9 left in the available pool
	assert(avail_buffer_pool.getBufferCount() == 8);

	/*
	 * Test decoding
	 */

	accl::BufferPool dec_buffer_pool(mss);

	// Grab single packet from the encoder buffer pool
	auto enc_buffer = enc_buffer_pool.pop();

	PacketDecoder decoder(mtu, &avail_buffer_pool, &dec_buffer_pool);
	decoder.decode(*enc_buffer);

	// Our decoded buffer pool should now have 1 packet in it
	assert(dec_buffer_pool.getBufferCount() == 1);
	// Our encoder buffer pool should have 0 packets in it
	assert(enc_buffer_pool.getBufferCount() == 0);


	/*
		const unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x08, 0x00, 0x45,
	   0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x9E, 0x4D, 0xAC, 0x10, 0x65, 0x66, 0xC0, 0xA8, 0x0A, 0x01, 0x30, 0x39,
	   0xD4, 0x31, 0x00, 0x6C, 0x7D, 0xEA, 0x41, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x42, 0x30, 0x31, 0x32,
	   0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x43, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x44, 0x30, 0x31,
	   0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x45, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x46, 0x30,
	   0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x47, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x48,
	   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x49, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	   0x4A};
	*/
	//std::string bin(reinterpret_cast<const char *>(data), sizeof(data));
	//REQUIRE(packet.compare(bin) == true);
}
