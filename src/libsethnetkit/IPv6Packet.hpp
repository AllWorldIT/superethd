#pragma once

#include "IPPacket.hpp"

#define SETH_PACKET_IPV6_IP_LEN 16

// IPv6 header definition
typedef struct __seth_packed {
		uint8_t version : 4;		// IP version (should be 6 for IPv6)
		uint8_t traffic_class;		// Traffic Class
		uint32_t flow_label : 20;	// Flow Label
		seth_be16_t payload_length; // Length of the payload
		uint8_t next_header;		// Identifies the type of the next header
		uint8_t hop_limit;			// Similar to TTL in IPv4
		uint8_t src_addr[16];		// Source IP address
		uint8_t dst_addr[16];		// Destination IP address
} ipv6_header_t;

class IPv6Packet : public IPPacket {
	protected:
		uint8_t traffic_class;
		uint32_t flow_label;
//		seth_be16_t payload_length;
		uint8_t next_header; // Type
		uint8_t hop_limit;
		std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> src_addr;
		std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> dst_addr;

	private:
		void _clear();

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

		uint16_t getHeaderOffset() const;
		uint16_t getHeaderSize() const;

		std::string asText() const;
		std::string asBinary() const;
};