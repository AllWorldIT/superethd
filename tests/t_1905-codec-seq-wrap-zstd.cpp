/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "debug.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "libaccl/buffer_pool.hpp"
#include "libaccl/logger.hpp"
#include "libsethnetkit/ethernet_packet.hpp"
#include "libtests/framework.hpp"
#include "packet_switch.hpp"
#include <cstdint>

TEST_CASE("Check sequence wrapping with ZSTD compression", "[codec]") {

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
	uint16_t buffer_size = l2mtu + (l2mtu / 10);
	std::shared_ptr<accl::BufferPool<PacketBuffer>> avail_buffer_pool =
		std::make_shared<accl::BufferPool<PacketBuffer>>(buffer_size, 9);
	std::shared_ptr<accl::BufferPool<PacketBuffer>> enc_buffer_pool = std::make_shared<accl::BufferPool<PacketBuffer>>(buffer_size);
	std::shared_ptr<accl::BufferPool<PacketBuffer>> dec_buffer_pool = std::make_shared<accl::BufferPool<PacketBuffer>>(buffer_size);

	std::string packet_bin = packet.asBinary();

	accl::logger.setLogLevel(accl::LogLevel::DEBUGGING);

	PacketEncoder encoder(l2mtu, l4mtu, enc_buffer_pool, avail_buffer_pool);
	encoder.setPacketFormat(PacketHeaderOptionFormatType::COMPRESSED_ZSTD);

	PacketDecoder decoder(l2mtu, dec_buffer_pool, avail_buffer_pool);

	// Set encoder sequence close to the end
	encoder.setSequence(UINT32_MAX - 5);

	for (uint64_t i = 0; i < 10; ++i) {
		std::unique_ptr<PacketBuffer> packet_buffer = avail_buffer_pool->pop();
		packet_buffer->clear();

		packet_buffer->append(packet_bin.data(), packet_bin.length());

		encoder.encode(std::move(packet_buffer));
		encoder.flush();

		// Make sure we now have a packet in the enc_buffer_pool
		REQUIRE(enc_buffer_pool->getBufferCount() == 1);

		/*
		 * Test decoding
		 */

		// Grab single packet from the encoder buffer pool
		auto enc_buffer = enc_buffer_pool->pop();

		decoder.decode(std::move(enc_buffer));

		// Our decoded buffer pool should now have 1 packet in it
		REQUIRE(dec_buffer_pool->getBufferCount() == 1);
		// Our encoder buffer pool should have 0 packets in it
		REQUIRE(enc_buffer_pool->getBufferCount() == 0);

		// Grab buffer from dec_buffer_pool
		auto dec_buffer = dec_buffer_pool->pop();

		// Now lets convert the buffer into a std::string and compare them
		std::string buffer_string(reinterpret_cast<const char *>(dec_buffer->getData()), dec_buffer->getDataSize());

		// Next we compare them...
		REQUIRE(buffer_string == packet_bin);

		avail_buffer_pool->push(std::move(dec_buffer));
	}

	// Check final sequences
	REQUIRE(encoder.getSequence() == 6);
	REQUIRE(decoder.getLastSequence() == 5);

	accl::logger.setLogLevel(accl::LogLevel::DEBUGGING);
}
