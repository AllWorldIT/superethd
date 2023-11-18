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
#include <cstdint>

TEST_CASE("Check sequence wrapping", "[codec]") {
	CERR("");
	CERR("");
	CERR("TEST: Check sequence wrapping");

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
	accl::BufferPool dec_buffer_pool(l2mtu);

	std::string packet_bin = packet.asBinary();

	accl::logger.setLogLevel(accl::LogLevel::DEBUGGING);

	PacketEncoder encoder(l2mtu, l4mtu, &avail_buffer_pool, &enc_buffer_pool);
	PacketDecoder decoder(l2mtu, &avail_buffer_pool, &dec_buffer_pool);

	// Set encoder sequence close to the end
	encoder.setSequence(UINT32_MAX - 5);

	for (uint64_t i = 0; i < 10; ++i) {
		std::unique_ptr<accl::Buffer> packet_buffer = avail_buffer_pool.pop();
		packet_buffer->clear();

		packet_buffer->append(packet_bin.data(), packet_bin.length());


		encoder.encode(std::move(packet_buffer));
		encoder.flush();

		// Make sure we now have a packet in the enc_buffer_pool
		assert(enc_buffer_pool.getBufferCount() == 1);

		/*
		 * Test decoding
		 */

		// Grab single packet from the encoder buffer pool
		auto enc_buffer = enc_buffer_pool.pop();

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

		avail_buffer_pool.push(std::move(dec_buffer));
	}

	// Check final sequences
	assert(encoder.getSequence() == 6);
	assert(decoder.getLastSequence() == 5);

	accl::logger.setLogLevel(accl::LogLevel::DEBUGGING);
}
