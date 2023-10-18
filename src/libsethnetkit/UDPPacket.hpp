#pragma once

#include "IPv4Packet.hpp"
#include "IPv6Packet.hpp"

typedef struct __seth_packed {
		seth_be16_t src_port; // Source port
		seth_be16_t dst_port; // Destination port
		seth_be16_t length;	  // Length of the UDP header and payload
		seth_be16_t checksum; // Checksum
} udp_header_t;

template <typename T>
concept UDPAllowedType = std::is_same_v<T, IPv4Packet> || std::is_same_v<T, IPv6Packet>;

template <UDPAllowedType T> class UDPPacketTmpl : public T {
	protected:
		seth_be16_t src_port;
		seth_be16_t dst_port;
		seth_be16_t checksum;

	private:
		void _clear();

	public:
		UDPPacketTmpl();
		UDPPacketTmpl(const std::vector<uint8_t> &data);

		~UDPPacketTmpl();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		uint16_t getSrcPort() const;
		void setSrcPort(uint16_t newSrcPort);

		uint16_t getDstPort() const;
		void setDstPort(uint16_t newDstPort);

		uint16_t getChecksum() const;

		uint16_t getHeaderOffset() const;
		uint16_t getHeaderSize() const;

		uint16_t getPacketSize() const;

		std::string asText() const;
		std::string asBinary() const;
};

// Define types we plan to use
template class UDPPacketTmpl<IPv4Packet>;
template class UDPPacketTmpl<IPv6Packet>;

// Create class aliases
using UDPv4Packet = UDPPacketTmpl<IPv4Packet>;
using UDPv6Packet = UDPPacketTmpl<IPv6Packet>;
