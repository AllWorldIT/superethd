/*
 * Ethernet packet handling.
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

#include "Packet.hpp"

#define SETH_PACKET_ETHERNET_HEADER_LEN 14
#define SETH_PACKET_ETHERNET_MAC_LEN 6

typedef struct __seth_packed {
		uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN]; // Destination MAC address
		uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN]; // Source MAC address
		seth_be16_t ethertype;						   // Ethertype field to indicate the protocol
} ethernet_header_t;

class EthernetPacket : public Packet {
	protected:
		std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac; // Destination MAC address
		std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac; // Source MAC address
		seth_be16_t ethertype;														// Ethertype field to indicate the protocol

	private:
		void _clear();

	public:
		EthernetPacket();
		EthernetPacket(const std::vector<uint8_t> &data);

		~EthernetPacket();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> getDstMac() const;
		void setDstMac(const std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> &mac);

		std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> getSrcMac() const;
		void setSrcMac(const std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> &mac);

		uint16_t getEthertype() const;
		void setEthertype(const uint16_t type);

		uint16_t getHeaderOffset() const override;
		uint16_t getHeaderSize() const override;

		std::string asText() const override;
		std::string asBinary() const override;
};
