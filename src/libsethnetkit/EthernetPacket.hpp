#pragma once

#include "Packet.hpp"

#define SETH_PACKET_ETHERNET_HEADER_LEN 14
#define SETH_PACKET_ETHERNET_MAC_LEN 6

typedef struct __seth_packed {
		uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN]; // Destination MAC address
		uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN]; // Source MAC address
		seth_be16_t ethertype;						   // Ethertype field to indicate the protocol
} ethernet_header_t;

class EthernetPacket : public Packet {
	protected:
		std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> dst_mac = {0, 0, 0, 0, 0, 0}; // Destination MAC address
		std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> src_mac = {0, 0, 0, 0, 0, 0}; // Source MAC address
		seth_be16_t ethertype = 0;														// Ethertype field to indicate the protocol
	public:
		EthernetPacket();
		EthernetPacket(const std::vector<uint8_t> &data);

		void parse(const std::vector<uint8_t> &data);

		std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> getDstMac() const;
		void setDstMac(const std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> &mac);

		std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> getSrcMac() const;
		void setSrcMac(const std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> &mac);

		uint16_t getEthertype() const;
		void setEthertype(const uint16_t type);

		std::string asText() const;
		std::string asBinary() const;
};
