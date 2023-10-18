/*
 * Packet buffer handling.
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

#include "packet.hpp"
#include <cstdint>
#include <vector>

class PacketBuffer {
	private:
		std::vector<uint8_t> data;

	public:
		PacketBuffer();
		PacketBuffer(uint16_t size);

		// Getter for size
		uint16_t getSize() const;
		// Getter for pointer to data
		const uint8_t *getPointer() const;
		// Method to print the buffer's content
		void printHex() const;

		// Method to compare our packet
		int compare(void *cmp, uint16_t len);

		// Method to copy data into the buffer
		bool copyData(const uint8_t *src, uint16_t len);
		// Method to resize the buffer
		void resize(uint16_t newSize);

		std::string asHex() const;
		ethernet_header_t *getHeaderEthernet();
		uint16_t getEthernetHeaderSize();
		ip_header_t *getHeaderIP();
		ipv4_header_t *getHeaderIPv4();

		void ethernetHeader(uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN], uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN],
							uint16_t ethertype);
		void IPHeader(uint16_t id, uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN], uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN]);
};