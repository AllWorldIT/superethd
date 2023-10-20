/*
 * UDP packet handling.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "IPv4Packet.hpp"
#include "IPv6Packet.hpp"

#define SETH_PACKET_IP_PROTOCOL_UDP 17

typedef struct __seth_packed {
		seth_be16_t src_port; // Source port
		seth_be16_t dst_port; // Destination port
		seth_be16_t length;	  // Length of the UDP header and payload
		seth_be16_t checksum; // Checksum
} udp_header_t;

template <typename T>
concept UDPAllowedType = std::is_same_v<T, IPv4Packet> || std::is_same_v<T, IPv6Packet>;

template <UDPAllowedType T> class UDPPacketTmpl : public T {
	protected:
		seth_be16_t src_port;
		seth_be16_t dst_port;
		seth_be16_t checksum;

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
