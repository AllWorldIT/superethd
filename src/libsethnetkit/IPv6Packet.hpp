/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "IPPacket.hpp"

inline constexpr uint16_t SETH_PACKET_ETHERTYPE_ETHERNET_IPV6{0x86DD};

inline constexpr uint16_t SETH_PACKET_IPV6_IP_LEN{16};

// IPv6 header definition
struct ipv6_header_t {
#if SETH_BYTE_ORDER == SETH_BIG_ENDIAN
		uint8_t version : 4;  // IP version (should be 6 for IPv6)
		uint8_t priority : 4; // Traffic Class => priority
#else
		uint8_t priority : 4; // Traffic Class => priority
		uint8_t version : 4;  // IP version (should be 6 for IPv6)
#endif
		uint32_t flow_label : 24;	// Flow Label
		seth_be16_t payload_length; // Length of the payload
		uint8_t next_header;		// Identifies the type of the next header
		uint8_t hop_limit;			// Similar to TTL in IPv4
		uint8_t src_addr[16];		// Source IP address
		uint8_t dst_addr[16];		// Destination IP address
} SETH_PACKED_ATTRIBUTES;

class IPv6Packet : public IPPacket {
	protected:
		uint8_t priority;
		uint32_t flow_label;
		//		seth_be16_t payload_length;
		uint8_t next_header; // Type
		uint8_t hop_limit;
		std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> src_addr;
		std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> dst_addr;

	private:
		void _clear();
		uint16_t _ipv6HeaderPayloadLength() const;

	public:
		IPv6Packet();
		IPv6Packet(const std::vector<uint8_t> &data);

		~IPv6Packet();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		uint8_t getTrafficClass() const;
		void setTrafficClass(uint8_t newTrafficClass);

		uint8_t getFlowLabel() const;
		void setFlowLabel(uint8_t newFlowLabel);

		uint16_t getPayloadLength() const;

		uint8_t getNextHeader() const;
		void setNextHeader(uint8_t newNextHeader);

		uint8_t getHopLimit() const;
		void setHopLimit(uint8_t newHopLimit);

		std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> getDstAddr() const;
		void setDstAddr(std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> newDstAddr);

		std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> getSrcAddr() const;
		void setSrcAddr(std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> newSrcAddr);

		uint32_t getPseudoChecksumLayer3(uint16_t length) const;

		uint16_t getHeaderSize() const override;
		uint16_t getHeaderSizeTotal() const override;
		uint16_t getLayer3Size() const override;

		std::string asText() const override;
		std::string asBinary() const override;
};