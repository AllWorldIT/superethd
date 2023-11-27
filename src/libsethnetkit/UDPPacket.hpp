/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "IPv4Packet.hpp"
#include "IPv6Packet.hpp"

inline constexpr uint8_t SETH_PACKET_IP_PROTOCOL_UDP{17};

struct udp_header_t {
		accl::be16_t src_port; // Source port
		accl::be16_t dst_port; // Destination port
		accl::be16_t length;   // Length of the UDP header and payload
		accl::be16_t checksum; // Checksum
} ACCL_PACKED_ATTRIBUTES;

template <typename T>
concept UDPAllowedType = std::is_same_v<T, IPv4Packet> || std::is_same_v<T, IPv6Packet>;

template <UDPAllowedType T> class UDPPacketTmpl : public T {
	protected:
		accl::be16_t src_port;
		accl::be16_t dst_port;
		accl::be16_t checksum;

	private:
		void _clear();

	public:
		UDPPacketTmpl();
		UDPPacketTmpl(const std::vector<uint8_t> &data);

		~UDPPacketTmpl();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		uint16_t getSrcPort() const;
		void setSrcPort(uint16_t newSrcPort);

		uint16_t getDstPort() const;
		void setDstPort(uint16_t newDstPort);

		uint16_t getChecksumLayer4() const;

		uint16_t getHeaderOffset() const override;
		uint16_t getHeaderSize() const override;
		uint16_t getHeaderSizeTotal() const override;
		uint16_t getLayer4Size() const;

		std::string asText() const override;
		std::string asBinary() const override;
};

// Define types we plan to use
template class UDPPacketTmpl<IPv4Packet>;
template class UDPPacketTmpl<IPv6Packet>;

// Create class aliases
using UDPv4Packet = UDPPacketTmpl<IPv4Packet>;
using UDPv6Packet = UDPPacketTmpl<IPv6Packet>;
