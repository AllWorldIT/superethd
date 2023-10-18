#pragma once

#include "IPv6Packet.hpp"

typedef struct __seth_packed {
		uint8_t type;		  // Type
		uint8_t code;		  // Code
		seth_be16_t checksum; // Checksum
		uint32_t unused;	  // Unused
} icmp6_header_t;

class ICMPv6Packet : public IPv6Packet {
	protected:
		uint8_t type;
		uint8_t code;
		seth_be16_t checksum;

	private:
		void _clear();

	public:
		ICMPv6Packet();
		ICMPv6Packet(const std::vector<uint8_t> &data);

		~ICMPv6Packet();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		uint8_t getType() const;
		void setType(uint8_t newType);

		uint8_t getCode() const;
		void setCode(uint8_t newCode);

		uint8_t getChecksum() const;

		uint16_t getHeaderOffset() const;
		uint16_t getHeaderSize() const;

		uint16_t getPacketSize() const;

		std::string asText() const;
		std::string asBinary() const;
};
