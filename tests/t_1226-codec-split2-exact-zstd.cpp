/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "Decoder.hpp"
#include "Encoder.hpp"
#include "debug.hpp"
#include "libaccl/BufferPool.hpp"
#include "libaccl/Logger.hpp"
#include "libsethnetkit/EthernetPacket.hpp"
#include "libtests/framework.hpp"

TEST_CASE("Check encoding 2 packets, where the second is split between encapsulated packets and fills the second one with ZSTD "
		  "compression",
		  "[codec]") {
	CERR("");
	CERR("");
	CERR("TEST: Check encoding 2 packets, where the second is split between encapsulated packets and fills the second one");

	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};

	int payload_size = 1301;
	std::vector<uint8_t> payloadBytes;
	const unsigned int seed = 12345;			   // Fixed seed for reproducibility
	std::mt19937 eng(seed);						   // Seed the generator with a fixed value
	std::uniform_int_distribution<> distr(0, 255); // Define the range
	for (int n = 0; n < payload_size; ++n) {
		payloadBytes.push_back(static_cast<uint8_t>(distr(eng))); // Generate a random byte and add to the vector
	}
	std::string payloadString(payloadBytes.begin(), payloadBytes.end());
	// Second packet
	int payload_size2 = 1465;
	std::vector<uint8_t> payloadBytes2;
	const unsigned int seed2 = 54321;				// Fixed seed for reproducibility
	std::mt19937 eng2(seed2);						// Seed the generator with a fixed value
	std::uniform_int_distribution<> distr2(0, 255); // Define the range
	for (int n = 0; n < payload_size2; ++n) {
		payloadBytes2.push_back(static_cast<uint8_t>(distr2(eng2))); // Generate a random byte and add to the vector
	}
	std::string payloadString2(payloadBytes2.begin(), payloadBytes2.end());

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

	packet2.addPayload(payloadBytes2);

	packet2.printText();
	packet2.printHex();

	/*
	 * Test encoding
	 */

	// Lets fire up the encoder
	uint16_t l2mtu = get_l2mtu_from_mtu(1500);
	uint16_t l4mtu = 1500 - 20 - 8; // IPv6 is 40
	uint16_t buffer_size = l2mtu + (l2mtu / 10);
	accl::BufferPool<PacketBuffer> avail_buffer_pool(buffer_size, 8);
	accl::BufferPool<PacketBuffer> enc_buffer_pool(buffer_size);

	std::string packet1_bin = packet1.asBinary();
	std::unique_ptr<PacketBuffer> packet1_buffer = std::make_unique<PacketBuffer>(buffer_size);
	packet1_buffer->append(packet1_bin.data(), packet1_bin.length());

	std::string packet2_bin = packet2.asBinary();
	std::unique_ptr<PacketBuffer> packet2_buffer = std::make_unique<PacketBuffer>(buffer_size);
	packet2_buffer->append(packet2_bin.data(), packet2_bin.length());

	PacketEncoder encoder(l2mtu, l4mtu, &avail_buffer_pool, &enc_buffer_pool);
	encoder.setPacketFormat(PacketHeaderOptionFormatType::COMPRESSED_ZSTD);
	encoder.encode(std::move(packet1_buffer));
	encoder.encode(std::move(packet2_buffer));
	encoder.flush();

	// Make sure we now have a packet in the enc_buffer_pool
	REQUIRE(enc_buffer_pool.getBufferCount() == 2);
	// We should have 9 left in the available pool
	REQUIRE(avail_buffer_pool.getBufferCount() == 6);

	/*
	 * Test decoding
	 */

	accl::BufferPool<PacketBuffer> dec_buffer_pool(buffer_size);

	// Grab single packet from the encoder buffer pool
	auto enc_buffer1 = enc_buffer_pool.pop();
	auto enc_buffer2 = enc_buffer_pool.pop();

	PacketDecoder decoder(l2mtu, &avail_buffer_pool, &dec_buffer_pool);
	decoder.decode(std::move(enc_buffer1));
	decoder.decode(std::move(enc_buffer2));

	// Our decoded buffer pool should now have 2 packets in it
	REQUIRE(dec_buffer_pool.getBufferCount() == 2);
	// Our encoder buffer pool should have 0 packets in it
	REQUIRE(enc_buffer_pool.getBufferCount() == 0);

	// Grab buffer from dec_buffer_pool
	auto dec_buffer1 = dec_buffer_pool.pop();
	auto dec_buffer2 = dec_buffer_pool.pop();

	// Now lets convert the buffer into a std::string and compare them
	std::string buffer1_string(reinterpret_cast<const char *>(dec_buffer1->getData()), dec_buffer1->getDataSize());
	std::string buffer2_string(reinterpret_cast<const char *>(dec_buffer2->getData()), dec_buffer2->getDataSize());
	// Next we compare them...
	REQUIRE(buffer1_string == packet1_bin);
	REQUIRE(buffer2_string == packet2_bin);
}
