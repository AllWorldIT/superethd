/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "debug.hpp"
#include "libaccl/BufferPool.hpp"
#include "libsethnetkit/EthernetPacket.hpp"
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
	uint16_t mtu = {1500};
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

	// Grab buffer from dec_buffer_pool
	auto dec_buffer = dec_buffer_pool.pop();

	// Now lets convert the buffer into a std::string and compare them
	std::string buffer_string(reinterpret_cast<const char *>(dec_buffer->getData()), dec_buffer->getDataSize());
	// Next we compare them...
	assert(buffer_string == packet_bin);
}

TEST_CASE("Check encoding of a packet that does not fit into MSS", "[codec]") {
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};
	accl::SequenceDataGenerator payloadSeq = accl::SequenceDataGenerator(1472);

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
	uint16_t mtu = {1500};
	uint16_t mss = mtu - 20 - 8; // IPv6 is 40
	accl::BufferPool avail_buffer_pool(mtu + SETH_PACKET_ETHERNET_HEADER_LEN, 10);
	accl::BufferPool enc_buffer_pool(mtu + SETH_PACKET_ETHERNET_HEADER_LEN);

	std::string packet_bin = packet.asBinary();

	PacketEncoder encoder(mss, mtu, &avail_buffer_pool, &enc_buffer_pool);
	encoder.encode(reinterpret_cast<char *>(packet_bin.data()), packet_bin.length());

	// Make sure we now have a packet in the enc_buffer_pool
	assert(enc_buffer_pool.getBufferCount() == 2);
	// We should have 9 left in the available pool
	assert(avail_buffer_pool.getBufferCount() == 7);

	/*
	 * Test decoding
	 */

	accl::BufferPool dec_buffer_pool(mtu + SETH_PACKET_ETHERNET_HEADER_LEN);

	// Grab single packet from the encoder buffer pool
	auto enc_buffer1 = enc_buffer_pool.pop();
	auto enc_buffer2 = enc_buffer_pool.pop();

	PacketDecoder decoder(mtu, &avail_buffer_pool, &dec_buffer_pool);
	decoder.decode(*enc_buffer1);
	decoder.decode(*enc_buffer2);

	// Our decoded buffer pool should now have 1 packet in it
	assert(dec_buffer_pool.getBufferCount() == 1);
	// Our encoder buffer pool should have 0 packets in it
	assert(enc_buffer_pool.getBufferCount() == 0);

	// Grab buffer from dec_buffer_pool
	auto dec_buffer = dec_buffer_pool.pop();

	// Now lets convert the buffer into a std::string and compare them
	std::string buffer_string(reinterpret_cast<const char *>(dec_buffer->getData()), dec_buffer->getDataSize());
	// Next we compare them...
	assert(buffer_string == packet_bin);
}
