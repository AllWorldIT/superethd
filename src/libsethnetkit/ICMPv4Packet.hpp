/*
 * ICMPv4 packet handling.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include "IPv4Packet.hpp"

#define SETH_PACKET_IP_PROTOCOL_ICMP4 1

typedef struct __seth_packed {
		uint8_t type;		  // Type
		uint8_t code;		  // Code
		seth_be16_t checksum; // Checksum
		uint32_t unused;	  // Unused
} icmp_header_t;

typedef struct __seth_packed {
		uint8_t type;			// Type
		uint8_t code;			// Code
		seth_be16_t checksum;	// Checksum
		seth_be16_t identifier; // Identifier
		seth_be16_t sequence;	// Sequence
} icmp_echo_header_t;

class ICMPv4Packet : public IPv4Packet {
	protected:
		uint8_t type;
		uint8_t code;
		seth_be16_t checksum;

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
