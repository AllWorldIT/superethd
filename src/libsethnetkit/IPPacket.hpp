#pragma once

#include "EthernetPacket.hpp"

#define SETH_PACKET_IP_VERSION_IPV4 0x4
#define SETH_PACKET_IP_VERSION_IPV6 0x6

#define SETH_PACKET_IP_PROTOCOL_ICMP4 1
#define SETH_PACKET_IP_PROTOCOL_TCP 6
#define SETH_PACKET_IP_PROTOCOL_UDP 17
#define SETH_PACKET_IP_PROTOCOL_ICMP6 58

// IP header definition
typedef struct __seth_packed {
#if SETH_BYTE_ORDER == SETH_BIG_ENDIAN
		uint8_t version : 4; // IP version (should be 4 for IPv4)
		uint8_t unused : 4;	 // Unused bits
#else
		uint8_t unused : 4;	 // Unused bits
		uint8_t version : 4; // IP version (should be 4 for IPv4)
#endif
} ip_header_t;

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

		uint16_t getHeaderOffset() const;

			std::string asText() const;
		std::string asBinary() const;
};
