/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "EthernetPacket.hpp"

inline constexpr uint8_t SETH_PACKET_IP_VERSION_IPV4 = 0x4;
inline constexpr uint8_t SETH_PACKET_IP_VERSION_IPV6 = 0x6;

// IP header definition
struct ip_header_t : public SETH_PackedAttributes {
#if SETH_BYTE_ORDER == SETH_BIG_ENDIAN
		uint8_t version : 4; // IP version (should be 4 for IPv4)
		uint8_t unused : 4;	 // Unused bits
#else
		uint8_t unused : 4;	 // Unused bits
		uint8_t version : 4; // IP version (should be 4 for IPv4)
#endif
};

class IPPacket : public EthernetPacket {
	protected:
		uint8_t version; // IP packet version

	private:
		void _clear();

	public:
		IPPacket();
		IPPacket(const std::vector<uint8_t> &data);

		~IPPacket();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		uint8_t getVersion() const;
		void setVersion(uint8_t newVersion);

		uint16_t getHeaderOffset() const override;
		virtual uint16_t getHeaderSizeTotal() const = 0;
		virtual uint16_t getLayer3Size() const = 0;

		std::string asText() const override;
		std::string asBinary() const override;
};
