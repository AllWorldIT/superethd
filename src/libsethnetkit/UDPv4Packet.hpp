#pragma once

#include "IPv4Packet.hpp"

typedef struct __seth_packed {
		seth_be16_t src_port; // Source port
		seth_be16_t dst_port; // Destination port
		seth_be16_t length;	  // Length of the UDP header and payload
		seth_be16_t checksum; // Checksum
} udp_header_t;

class UDPv4Packet : public IPv4Packet {
	protected:
		seth_be16_t src_port;
		seth_be16_t dst_port;
		seth_be16_t length;
		seth_be16_t checksum;

	public:
		UDPv4Packet(const std::vector<uint8_t> &data);

		void parse(const std::vector<uint8_t> &data);

		uint16_t getSrcPort() const;
		void setSrcPort(uint16_t newSrcPort);

		uint16_t getDstPort() const;
		void setDstPort(uint16_t newDstPort);

		uint16_t getLength() const;

		uint16_t getChecksum() const;

		std::string asText() const;
		std::string asBinary() const;
};
