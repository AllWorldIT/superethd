/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "debug.hpp"
#include "libaccl/BufferPool.hpp"
#include "libaccl/Logger.hpp"
#include "libsethnetkit/EthernetPacket.hpp"
#include "libtests/framework.hpp"
#include <memory>

TEST_CASE("Benchmark codec", "[codec]") {
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

	accl::BufferPool buffer_pool(l2mtu, 4);

	accl::BufferPool encoded_pool(l2mtu);
	accl::BufferPool decoded_pool(l2mtu);

	std::string packet_bin = packet.asBinary();

	PacketEncoder encoder(l2mtu, l4mtu, &buffer_pool, &encoded_pool);
	PacketDecoder decoder(l4mtu, &buffer_pool, &decoded_pool);

	auto prev_level = accl::logger.getLogLevel();
	accl::logger.setLogLevel(accl::LogLevel::ERROR);

	// Disable debugging
	BENCHMARK("Encode decode one packet") {
		std::unique_ptr<accl::Buffer> packet_buffer, encoder_buffer, decoder_buffer;

		for (int i = 0; i < 100; ++i) {
			LOG_DEBUG_INTERNAL("LOOP: ", i);
			// Set up packet
			packet_buffer = buffer_pool.pop();
			packet_buffer->clear();
			packet_buffer->append(packet_bin.data(), packet_bin.length());

			// Encode packet
			encoder.encode(std::move(packet_buffer));
			encoder.flush();

			// Grab encoded packet and try decode
			encoder_buffer = encoded_pool.pop();
			decoder.decode(std::move(encoder_buffer));

			// Push buffers back onto available pool
			decoder_buffer = decoded_pool.pop();
			buffer_pool.push(std::move(decoder_buffer));
		}
	};

	accl::logger.setLogLevel(prev_level);
}
