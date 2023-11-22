/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
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

TEST_CASE("Check encoding of two packets into a single encapsulated packet with a sweeping third packet", "[codec]") {

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
	 * Third packet
	 */
	UDPv4Packet packet3;

	packet3.setDstMac(dst_mac);
	packet3.setSrcMac(src_mac);
	packet3.setDstAddr(dst_ip);
	packet3.setSrcAddr(src_ip);

	packet3.setSrcPort(23456);
	packet3.setDstPort(34567);

	std::vector<uint8_t> packet3_payload{0};

	/*
	 * Test encoding
	 */

	std::vector<uint8_t> ex(static_cast<uint8_t>('X'));

	accl::logger.setLogLevel(accl::LogLevel::INFO);

	for (int i = 1; i <= 1391; ++i) {
		ex.push_back(static_cast<uint8_t>('X'));

		packet3.addPayload(ex);

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

		std::string packet3_bin = packet3.asBinary();
		std::unique_ptr<PacketBuffer> packet3_buffer = std::make_unique<PacketBuffer>(buffer_size);
		packet3_buffer->append(packet3_bin.data(), packet3_bin.length());

		PacketEncoder encoder(l2mtu, l4mtu, &avail_buffer_pool, &enc_buffer_pool);
		encoder.encode(std::move(packet1_buffer));
		encoder.encode(std::move(packet2_buffer));
		encoder.encode(std::move(packet3_buffer));
		encoder.flush();

		/*
		 * Test decoding
		 */

		accl::BufferPool<PacketBuffer> dec_buffer_pool(buffer_size);

		// Grab single packet from the encoder buffer pool
		auto buffers = enc_buffer_pool.pop(0);

		PacketDecoder decoder(l2mtu, &avail_buffer_pool, &dec_buffer_pool);
		for (auto &buffer : buffers) {
			decoder.decode(std::move(buffer));
		}

		// Our decoded buffer pool should now have 2 packets in it
		REQUIRE(dec_buffer_pool.getBufferCount() == 3);
		// Our encoder buffer pool should have 0 packets in it
		REQUIRE(enc_buffer_pool.getBufferCount() == 0);

		// Grab buffer from dec_buffer_pool
		auto dec_buffer1 = dec_buffer_pool.pop();
		auto dec_buffer2 = dec_buffer_pool.pop();
		auto dec_buffer3 = dec_buffer_pool.pop();

		// Now lets convert the buffer into a std::string and compare them
		std::string buffer1_string(reinterpret_cast<const char *>(dec_buffer1->getData()), dec_buffer1->getDataSize());
		std::string buffer2_string(reinterpret_cast<const char *>(dec_buffer2->getData()), dec_buffer2->getDataSize());
		std::string buffer3_string(reinterpret_cast<const char *>(dec_buffer3->getData()), dec_buffer3->getDataSize());
		// Next we compare them...
		REQUIRE(buffer1_string == packet1_bin);
		REQUIRE(buffer2_string == packet2_bin);
		REQUIRE(buffer3_string == packet3_bin);
	}
}
