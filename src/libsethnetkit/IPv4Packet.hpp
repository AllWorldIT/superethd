#pragma once

#include "IPPacket.hpp"

#define SETH_PACKET_IPV4_HEADER_LEN 20
#define SETH_PACKET_IPV4_IP_LEN 4

// IPv4 header definition
typedef struct __seth_packed {
#if SETH_BYTE_ORDER == SETH_BIG_ENDIAN
		uint8_t version : 4; // IP version (should be 4 for IPv4)
		uint8_t ihl : 4;	 // Internet Header Length (header length in 32-bit words)
#else
		uint8_t ihl : 4;	 // Internet Header Length (header length in 32-bit words)
		uint8_t version : 4; // IP version (should be 4 for IPv4)
#endif
		uint8_t dscp : 6;		  // DSCP
		uint8_t ecn : 2;		  // DSCP
		seth_be16_t total_length; // Total packet length (header + data)
		seth_be16_t id;			  // Identification
		seth_be16_t frag_off;	  // Flags (3 bits) and Fragment Offset (13 bits)
		uint8_t ttl;			  // Time to Live
		uint8_t protocol;		  // Protocol (e.g., TCP, UDP)
		seth_be16_t checksum;	  // Header checksum
		uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN];
		uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN];
} ipv4_header_t;

class IPv4Packet : public IPPacket {
	protected:
		uint8_t ihl;
		uint8_t dscp;
		uint8_t ecn;
		seth_be16_t total_length;
		seth_be16_t id;
		seth_be16_t frag_off;
		uint8_t ttl;
		uint8_t protocol;
		seth_be16_t checksum;
		std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> src_addr;
		std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> dst_addr;

	private:
		void _clear();

	public:
		IPv4Packet();
		IPv4Packet(const std::vector<uint8_t> &data);

		~IPv4Packet();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		uint8_t getIHL() const;
		void setIHL(uint8_t newIHL);

		uint8_t getDSCP() const;
		void setDSCP(uint8_t newDSCP);

		uint8_t getECN() const;
		void setECN(uint8_t newECN);

		uint16_t getTotalLength() const;

		uint16_t getId() const;
		void setId(uint16_t newId);

		uint16_t getFragOff() const;
		void setFragOff(uint16_t newFragOff);

		uint8_t getTtl() const;
		void setTtl(uint8_t newTtl);

		uint8_t getProtocol() const;
		void setProtocol(uint8_t newProtocol);

		uint16_t getChecksum() const;
		void setChecksum(uint16_t newChecksum) const;

		std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> getDstAddr() const;
		void setDstAddr(std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> newDstAddr);

		std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> getSrcAddr() const;
		void setSrcAddr(std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> newSrcAddr);

		std::string asText() const;
		std::string asBinary() const;
};
