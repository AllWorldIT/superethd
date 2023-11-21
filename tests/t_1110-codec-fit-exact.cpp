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


TEST_CASE("Check encoding of a packet that will fit exactly into MSS", "[codec]") {
	CERR("");
	CERR("");
	CERR("TEST: Check encoding of a packet that will fit exactly into MSS");

	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_ip = {192, 168, 10, 1};
	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_ip = {172, 16, 101, 102};
	accl::SequenceDataGenerator payloadSeq = accl::SequenceDataGenerator(1414);

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
	accl::BufferPool<PacketBuffer> avail_buffer_pool(l2mtu, 9);
	accl::BufferPool<PacketBuffer> enc_buffer_pool(l2mtu);

	std::string packet_bin = packet.asBinary();
	std::unique_ptr<PacketBuffer> packet_buffer = std::make_unique<PacketBuffer>(l2mtu);
	packet_buffer->append(packet_bin.data(), packet_bin.length());

	PacketEncoder encoder(l2mtu, l4mtu, &avail_buffer_pool, &enc_buffer_pool);
	encoder.encode(std::move(packet_buffer));
	// NK: As we fill the packet, we should get automatically flushed
	//encoder.flush();

	// Make sure we now have a packet in the enc_buffer_pool
	REQUIRE(enc_buffer_pool.getBufferCount() == 1);
	// We should have 9 left in the available pool
	REQUIRE(avail_buffer_pool.getBufferCount() == 7);

	/*
	 * Test decoding
	 */

	accl::BufferPool<PacketBuffer> dec_buffer_pool(l2mtu);

	// Grab single packet from the encoder buffer pool
	auto enc_buffer = enc_buffer_pool.pop();

	// Make sure the encoded packet is the right size
	REQUIRE(enc_buffer->getDataSize() == 1472);

	PacketDecoder decoder(l2mtu, &avail_buffer_pool, &dec_buffer_pool);
	decoder.decode(std::move(enc_buffer));

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
