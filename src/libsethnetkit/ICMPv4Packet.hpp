#pragma once

#include "IPv4Packet.hpp"

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

	public:
		ICMPv4Packet(const std::vector<uint8_t> &data);

		void parse(const std::vector<uint8_t> &data);

		uint8_t getType() const;
		void setType(uint8_t newType);

		uint8_t getCode() const;
		void setCode(uint8_t newCode);

		uint8_t getChecksum() const;

		std::string asText() const;
		std::string asBinary() const;
};
