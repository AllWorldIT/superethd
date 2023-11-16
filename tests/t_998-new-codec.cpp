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
	CERR("TEST: Check encoding of a packet that will fit into MSS");

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
	uint16_t l2mtu = get_l2mtu_from_mtu(1500);
	uint16_t l4mtu = 1500 - 20 - 8; // IPv6 is 40
	accl::BufferPool avail_buffer_pool(l2mtu, 9);
	accl::BufferPool enc_buffer_pool(l2mtu);

	std::string packet_bin = packet.asBinary();
	std::unique_ptr<accl::Buffer> packet_buffer = std::make_unique<accl::Buffer>(l2mtu);
	packet_buffer->append(packet_bin.data(), packet_bin.length());

	PacketEncoder encoder(l2mtu, l4mtu, &avail_buffer_pool, &enc_buffer_pool);
	encoder.encode(std::move(packet_buffer));
	encoder.flush();

	// Make sure we now have a packet in the enc_buffer_pool
	assert(enc_buffer_pool.getBufferCount() == 1);
	// We should have 8 left in the available pool
	assert(avail_buffer_pool.getBufferCount() == 8);

	/*
	 * Test decoding
	 */

	accl::BufferPool dec_buffer_pool(l2mtu);

	// Grab single packet from the encoder buffer pool
	auto enc_buffer = enc_buffer_pool.pop();

	PacketDecoder decoder(l2mtu, &avail_buffer_pool, &dec_buffer_pool);
	decoder.decode(std::move(enc_buffer));

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

TEST_CASE("Check encoding of a packet that will fit exactly into MSS", "[codec]") {
	CERR("TEST: Check encoding of a packet that will fit exactly into MSS");

	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};
	accl::SequenceDataGenerator payloadSeq = accl::SequenceDataGenerator(1418);

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
	uint16_t l2mtu = get_l2mtu_from_mtu(1500);
	uint16_t l4mtu = 1500 - 20 - 8; // IPv6 is 40
	accl::BufferPool avail_buffer_pool(l2mtu, 9);
	accl::BufferPool enc_buffer_pool(l2mtu);

	std::string packet_bin = packet.asBinary();
	std::unique_ptr<accl::Buffer> packet_buffer = std::make_unique<accl::Buffer>(l2mtu);
	packet_buffer->append(packet_bin.data(), packet_bin.length());

	PacketEncoder encoder(l2mtu, l4mtu, &avail_buffer_pool, &enc_buffer_pool);
	encoder.encode(std::move(packet_buffer));
	encoder.flush();

	// Make sure we now have a packet in the enc_buffer_pool
	assert(enc_buffer_pool.getBufferCount() == 1);
	// We should have 9 left in the available pool
	assert(avail_buffer_pool.getBufferCount() == 8);

	/*
	 * Test decoding
	 */

	accl::BufferPool dec_buffer_pool(l2mtu);

	// Grab single packet from the encoder buffer pool
	auto enc_buffer = enc_buffer_pool.pop();

	// Make sure the encoded packet is the right size
	assert(enc_buffer->getDataSize() == 1472);

	PacketDecoder decoder(l2mtu, &avail_buffer_pool, &dec_buffer_pool);
	decoder.decode(std::move(enc_buffer));

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
	CERR("TEST: Check encoding of a packet that does not fit into MSS");

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
	uint16_t l2mtu = get_l2mtu_from_mtu(1500);
	uint16_t l4mtu = 1500 - 20 - 8; // IPv6 is 40
	accl::BufferPool avail_buffer_pool(l2mtu, 10);
	accl::BufferPool enc_buffer_pool(l2mtu);

	std::string packet_bin = packet.asBinary();
	std::unique_ptr<accl::Buffer> packet_buffer = std::make_unique<accl::Buffer>(l2mtu);
	packet_buffer->append(packet_bin.data(), packet_bin.length());

	PacketEncoder encoder(l2mtu, l4mtu, &avail_buffer_pool, &enc_buffer_pool);
	encoder.encode(std::move(packet_buffer));
	encoder.flush();

	// Make sure we now have a packet in the enc_buffer_pool
	assert(enc_buffer_pool.getBufferCount() == 2);
	// We should have 8 left in the available pool
	assert(avail_buffer_pool.getBufferCount() == 8);

	/*
	 * Test decoding
	 */

	accl::BufferPool dec_buffer_pool(l2mtu);

	// Grab single packet from the encoder buffer pool
	auto enc_buffer1 = enc_buffer_pool.pop();
	auto enc_buffer2 = enc_buffer_pool.pop();

	PacketDecoder decoder(l2mtu, &avail_buffer_pool, &dec_buffer_pool);
	decoder.decode(std::move(enc_buffer1));
	decoder.decode(std::move(enc_buffer2));

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

TEST_CASE("Check encoding of two packets into a single encapsulated packet", "[codec]") {
	CERR("TEST: Check encoding of two packets into a single encapsulated packet");

	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};
	accl::SequenceDataGenerator payloadSeq = accl::SequenceDataGenerator(100);

	std::string payloadString = payloadSeq.asString();
	std::vector<uint8_t> payloadBytes = payloadSeq.asBytes();

	/*
	 * First packet
	 */
	UDPv4Packet packet1;

	packet1.setDstMac(dst_mac);
	packet1.setSrcMac(src_mac);
	packet1.setDstAddr(dst_ip);
	packet1.setSrcAddr(src_ip);

	packet1.setSrcPort(12345);
	packet1.setDstPort(54321);

	packet1.addPayload(payloadBytes);

	packet1.printText();
	packet1.printHex();

	/*
	 * Second packet
	 */
	UDPv4Packet packet2;

	packet2.setDstMac(dst_mac);
	packet2.setSrcMac(src_mac);
	packet2.setDstAddr(dst_ip);
	packet2.setSrcAddr(src_ip);

	packet2.setSrcPort(56789);
	packet2.setDstPort(65432);

	packet2.addPayload(payloadBytes);

	packet2.printText();
	packet2.printHex();

	/*
	 * Test encoding
	 */

	// Lets fire up the encoder
	uint16_t l2mtu = get_l2mtu_from_mtu(1500);
	uint16_t l4mtu = 1500 - 20 - 8; // IPv6 is 40
	accl::BufferPool avail_buffer_pool(l2mtu, 8);
	accl::BufferPool enc_buffer_pool(l2mtu);

	std::string packet1_bin = packet1.asBinary();
	std::unique_ptr<accl::Buffer> packet1_buffer = std::make_unique<accl::Buffer>(l2mtu);
	packet1_buffer->append(packet1_bin.data(), packet1_bin.length());

	std::string packet2_bin = packet1.asBinary();
	std::unique_ptr<accl::Buffer> packet2_buffer = std::make_unique<accl::Buffer>(l2mtu);
	packet2_buffer->append(packet2_bin.data(), packet2_bin.length());

	PacketEncoder encoder(l2mtu, l4mtu, &avail_buffer_pool, &enc_buffer_pool);
	encoder.encode(std::move(packet1_buffer));
	encoder.encode(std::move(packet2_buffer));
	encoder.flush();

	// Make sure we now have a packet in the enc_buffer_pool
	assert(enc_buffer_pool.getBufferCount() == 1);
	// We should have 8 left in the available pool
	assert(avail_buffer_pool.getBufferCount() == 8);

	/*
	 * Test decoding
	 */

	accl::BufferPool dec_buffer_pool(l2mtu);

	// Grab single packet from the encoder buffer pool
	auto enc_buffer = enc_buffer_pool.pop();

	PacketDecoder decoder(l2mtu, &avail_buffer_pool, &dec_buffer_pool);
	decoder.decode(std::move(enc_buffer));

	// Our decoded buffer pool should now have 2 packets in it
	assert(dec_buffer_pool.getBufferCount() == 2);
	// Our encoder buffer pool should have 0 packets in it
	assert(enc_buffer_pool.getBufferCount() == 0);

	// Grab buffer from dec_buffer_pool
	auto dec_buffer1 = dec_buffer_pool.pop();
	auto dec_buffer2 = dec_buffer_pool.pop();

	// Now lets convert the buffer into a std::string and compare them
	std::string buffer1_string(reinterpret_cast<const char *>(dec_buffer1->getData()), dec_buffer1->getDataSize());
	std::string buffer2_string(reinterpret_cast<const char *>(dec_buffer2->getData()), dec_buffer2->getDataSize());
	// Next we compare them...
	assert(buffer1_string == packet1_bin);
	assert(buffer2_string == packet2_bin);
}

TEST_CASE("Check encoding of two packets into a single encapsulated packet exactly", "[codec]") {
	CERR("TEST: Check encoding of two packets into a single encapsulated packet exactly");

	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};
	accl::SequenceDataGenerator payloadSeq = accl::SequenceDataGenerator(686);

	std::string payloadString = payloadSeq.asString();
	std::vector<uint8_t> payloadBytes = payloadSeq.asBytes();

	/*
	 * First packet
	 */
	UDPv4Packet packet1;

	packet1.setDstMac(dst_mac);
	packet1.setSrcMac(src_mac);
	packet1.setDstAddr(dst_ip);
	packet1.setSrcAddr(src_ip);

	packet1.setSrcPort(12345);
	packet1.setDstPort(54321);

	packet1.addPayload(payloadBytes);

	packet1.printText();
	packet1.printHex();

	/*
	 * Second packet
	 */
	UDPv4Packet packet2;

	packet2.setDstMac(dst_mac);
	packet2.setSrcMac(src_mac);
	packet2.setDstAddr(dst_ip);
	packet2.setSrcAddr(src_ip);

	packet2.setSrcPort(56789);
	packet2.setDstPort(65432);

	packet2.addPayload(payloadBytes);

	packet2.printText();
	packet2.printHex();

	/*
	 * Test encoding
	 */

	// Lets fire up the encoder
	uint16_t l2mtu = get_l2mtu_from_mtu(1500);
	uint16_t l4mtu = 1500 - 20 - 8; // IPv6 is 40
	accl::BufferPool avail_buffer_pool(l2mtu, 8);
	accl::BufferPool enc_buffer_pool(l2mtu);

	std::string packet1_bin = packet1.asBinary();
	std::unique_ptr<accl::Buffer> packet1_buffer = std::make_unique<accl::Buffer>(l2mtu);
	packet1_buffer->append(packet1_bin.data(), packet1_bin.length());

	std::string packet2_bin = packet1.asBinary();
	std::unique_ptr<accl::Buffer> packet2_buffer = std::make_unique<accl::Buffer>(l2mtu);
	packet2_buffer->append(packet2_bin.data(), packet2_bin.length());

	PacketEncoder encoder(l2mtu, l4mtu, &avail_buffer_pool, &enc_buffer_pool);
	encoder.encode(std::move(packet1_buffer));
	encoder.encode(std::move(packet2_buffer));
	encoder.flush();

	// Make sure we now have a packet in the enc_buffer_pool
	assert(enc_buffer_pool.getBufferCount() == 1);
	// We should have 8 left in the available pool
	assert(avail_buffer_pool.getBufferCount() == 8);

	/*
	 * Test decoding
	 */

	accl::BufferPool dec_buffer_pool(l2mtu);

	// Grab single packet from the encoder buffer pool
	auto enc_buffer = enc_buffer_pool.pop();

	assert(enc_buffer->getDataSize() == 1472);

	PacketDecoder decoder(l2mtu, &avail_buffer_pool, &dec_buffer_pool);
	decoder.decode(std::move(enc_buffer));

	// Our decoded buffer pool should now have 2 packets in it
	assert(dec_buffer_pool.getBufferCount() == 2);
	// Our encoder buffer pool should have 0 packets in it
	assert(enc_buffer_pool.getBufferCount() == 0);

	// Grab buffer from dec_buffer_pool
	auto dec_buffer1 = dec_buffer_pool.pop();
	auto dec_buffer2 = dec_buffer_pool.pop();

	// Now lets convert the buffer into a std::string and compare them
	std::string buffer1_string(reinterpret_cast<const char *>(dec_buffer1->getData()), dec_buffer1->getDataSize());
	std::string buffer2_string(reinterpret_cast<const char *>(dec_buffer2->getData()), dec_buffer2->getDataSize());
	// Next we compare them...
	assert(buffer1_string == packet1_bin);
	assert(buffer2_string == packet2_bin);
}

TEST_CASE("Check encoding 2 packets, where the second is split between encapsulated packets", "[codec]") {
	CERR("TEST: Check encoding 2 packets, where the second is split between encapsulated packets");

	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};
	accl::SequenceDataGenerator payloadSeq = accl::SequenceDataGenerator(687);

	std::string payloadString = payloadSeq.asString();
	std::vector<uint8_t> payloadBytes = payloadSeq.asBytes();

	/*
	 * First packet
	 */
	UDPv4Packet packet1;

	packet1.setDstMac(dst_mac);
	packet1.setSrcMac(src_mac);
	packet1.setDstAddr(dst_ip);
	packet1.setSrcAddr(src_ip);

	packet1.setSrcPort(12345);
	packet1.setDstPort(54321);

	packet1.addPayload(payloadBytes);

	packet1.printText();
	packet1.printHex();

	/*
	 * Second packet
	 */
	UDPv4Packet packet2;

	packet2.setDstMac(dst_mac);
	packet2.setSrcMac(src_mac);
	packet2.setDstAddr(dst_ip);
	packet2.setSrcAddr(src_ip);

	packet2.setSrcPort(56789);
	packet2.setDstPort(65432);

	packet2.addPayload(payloadBytes);

	packet2.printText();
	packet2.printHex();

	/*
	 * Test encoding
	 */

	// Lets fire up the encoder
	uint16_t l2mtu = get_l2mtu_from_mtu(1500);
	uint16_t l4mtu = 1500 - 20 - 8; // IPv6 is 40
	accl::BufferPool avail_buffer_pool(l2mtu, 8);
	accl::BufferPool enc_buffer_pool(l2mtu);

	std::string packet1_bin = packet1.asBinary();
	std::unique_ptr<accl::Buffer> packet1_buffer = std::make_unique<accl::Buffer>(l2mtu);
	packet1_buffer->append(packet1_bin.data(), packet1_bin.length());

	std::string packet2_bin = packet1.asBinary();
	std::unique_ptr<accl::Buffer> packet2_buffer = std::make_unique<accl::Buffer>(l2mtu);
	packet2_buffer->append(packet2_bin.data(), packet2_bin.length());

	PacketEncoder encoder(l2mtu, l4mtu, &avail_buffer_pool, &enc_buffer_pool);
	encoder.encode(std::move(packet1_buffer));
	encoder.encode(std::move(packet2_buffer));
	encoder.flush();

	// Make sure we now have a packet in the enc_buffer_pool
	assert(enc_buffer_pool.getBufferCount() == 2);
	// We should have 9 left in the available pool
	assert(avail_buffer_pool.getBufferCount() == 7);

	/*
	 * Test decoding
	 */

	accl::BufferPool dec_buffer_pool(l2mtu);

	// Grab single packet from the encoder buffer pool
	auto enc_buffer1 = enc_buffer_pool.pop();
	auto enc_buffer2 = enc_buffer_pool.pop();

	PacketDecoder decoder(l2mtu, &avail_buffer_pool, &dec_buffer_pool);
	decoder.decode(std::move(enc_buffer1));
	decoder.decode(std::move(enc_buffer2));

	// Our decoded buffer pool should now have 2 packets in it
	assert(dec_buffer_pool.getBufferCount() == 2);
	// Our encoder buffer pool should have 0 packets in it
	assert(enc_buffer_pool.getBufferCount() == 0);

	// Grab buffer from dec_buffer_pool
	auto dec_buffer1 = dec_buffer_pool.pop();
	auto dec_buffer2 = dec_buffer_pool.pop();

	// Now lets convert the buffer into a std::string and compare them
	std::string buffer1_string(reinterpret_cast<const char *>(dec_buffer1->getData()), dec_buffer1->getDataSize());
	std::string buffer2_string(reinterpret_cast<const char *>(dec_buffer2->getData()), dec_buffer2->getDataSize());
	// Next we compare them...
	assert(buffer1_string == packet1_bin);
	assert(buffer2_string == packet2_bin);
}
