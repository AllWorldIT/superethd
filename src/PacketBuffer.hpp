#pragma once

#include "packet.hpp"
#include <cstdint>
#include <vector>

class PacketBuffer {
	private:
		std::vector<uint8_t> data;

	public:
		PacketBuffer();
		PacketBuffer(uint16_t size);

		// Getter for size
		uint16_t getSize() const;
		// Getter for pointer to data
		const uint8_t *getPointer() const;
		// Method to print the buffer's content
		void printHex() const;

		// Method to compare our packet
		int compare(void *cmp, uint16_t len);

		// Method to copy data into the buffer
		bool copyData(const uint8_t *src, uint16_t len);
		// Method to resize the buffer
		void resize(uint16_t newSize);

		std::string asHex() const;
		ethernet_header_t *getHeaderEthernet();
		uint16_t getEthernetHeaderSize() ;
		ip_header_t *getHeaderIP();
		ipv4_header_t *getHeaderIPv4();

		void ethernetHeader(uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN],
																		 uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN],
																		 uint16_t ethertype);
		void IPHeader(uint16_t id, uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN], uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN]);
};