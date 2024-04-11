/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "../libaccl/endian.hpp"
#include "ipv4_packet.hpp"

inline constexpr uint8_t SETH_PACKET_IP_PROTOCOL_ICMP4{1};

struct icmp_header_t {
		uint8_t type;		   // Type
		uint8_t code;		   // Code
		accl::be16_t checksum; // Checksum
		uint32_t unused;	   // Unused
} ACCL_PACKED_ATTRIBUTES;

struct icmp_echo_header_t {
		uint8_t type;			 // Type
		uint8_t code;			 // Code
		accl::be16_t checksum;	 // Checksum
		accl::be16_t identifier; // Identifier
		accl::be16_t sequence;	 // Sequence
} ACCL_PACKED_ATTRIBUTES;

class ICMPv4Packet : public IPv4Packet {
	protected:
		uint8_t type;
		uint8_t code;
		accl::be16_t checksum;

	private:
		void _clear();

	public:
		ICMPv4Packet();
		ICMPv4Packet(const std::vector<uint8_t> &data);

		~ICMPv4Packet();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		uint8_t getType() const;
		void setType(uint8_t newType);

		uint8_t getCode() const;
		void setCode(uint8_t newCode);

		uint16_t getChecksumLayer4() const;

		uint16_t getHeaderOffset() const override;
		uint16_t getHeaderSize() const override;
		uint16_t getHeaderSizeTotal() const override;
		uint16_t getLayer4Size() const;

		std::string asText() const override;
		std::string asBinary() const override;
};
