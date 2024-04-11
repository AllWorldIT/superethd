/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "decoder.hpp"
#include "encoder.hpp"
#include "debug.hpp"
#include "libaccl/buffer_pool.hpp"
#include "libaccl/logger.hpp"
#include "libsethnetkit/ethernet_packet.hpp"
#include "libtests/framework.hpp"

TEST_CASE("Check encoding of a packet that does not fit into MSS with ZSTD compression", "[codec]") {

	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};

	std::vector<uint8_t> payloadBytes;
	const unsigned int seed = 12345;			   // Fixed seed for reproducibility
	std::mt19937 eng(seed);						   // Seed the generator with a fixed value
	std::uniform_int_distribution<> distr(0, 255); // Define the range
	for (int n = 0; n < 1472; ++n) {
		payloadBytes.push_back(static_cast<uint8_t>(distr(eng))); // Generate a random byte and add to the vector
	}
	std::string payloadString(payloadBytes.begin(), payloadBytes.end());

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
	accl::BufferPool<PacketBuffer> avail_buffer_pool(buffer_size, 10);
	accl::BufferPool<PacketBuffer> enc_buffer_pool(buffer_size);

	std::string packet_bin = packet.asBinary();
	std::unique_ptr<PacketBuffer> packet_buffer = std::make_unique<PacketBuffer>(buffer_size);
	packet_buffer->append(packet_bin.data(), packet_bin.length());

	PacketEncoder encoder(l2mtu, l4mtu, &avail_buffer_pool, &enc_buffer_pool);
	encoder.setPacketFormat(PacketHeaderOptionFormatType::COMPRESSED_ZSTD);
	encoder.encode(std::move(packet_buffer));
	encoder.flush();

	// Make sure we now have a packet in the enc_buffer_pool
	REQUIRE(enc_buffer_pool.getBufferCount() == 2);
	// We should have 8 left in the available pool
	REQUIRE(avail_buffer_pool.getBufferCount() == 7);

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

	// Our decoded buffer pool should now have 1 packet in it
	REQUIRE(dec_buffer_pool.getBufferCount() == 1);
	// Our encoder buffer pool should have 0 packets in it
	REQUIRE(enc_buffer_pool.getBufferCount() == 0);

	// Grab buffer from dec_buffer_pool
	auto dec_buffer = dec_buffer_pool.pop();

	// Now lets convert the buffer into a std::string and compare them
	std::string buffer_string(reinterpret_cast<const char *>(dec_buffer->getData()), dec_buffer->getDataSize());
	// Next we compare them...
	REQUIRE(buffer_string == packet_bin);
}
