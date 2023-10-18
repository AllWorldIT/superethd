/*
 * TCP packet handling.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include "IPv4Packet.hpp"
#include "IPv6Packet.hpp"

typedef struct __seth_packet {
		uint8_t cwr : 1;
		uint8_t ece : 1;
		uint8_t urg : 1;
		uint8_t ack : 1;
		uint8_t psh : 1;
		uint8_t rst : 1;
		uint8_t syn : 1;
		uint8_t fin : 1;
} tcp_options_header_t;

typedef struct __seth_packed {
		seth_be16_t src_port; // Source port
		seth_be16_t dst_port; // Destination port

		seth_be32_t sequence; // Sequence

		seth_be32_t ack; // Acknowledgment if ACK set

#if SETH_BYTE_ORDER == SETH_BIG_ENDIAN
		uint8_t data_offset : 4; // Data offset
		uint8_t reserved : 4;	 // Reserved bits
#else
		uint8_t reserved : 4; // Reserved bits
		uint8_t ihl : 4;	  // Data offset
#endif

		tcp_options_header_t options; // TCP optoins
		seth_be16_t window;			  // Window size

		seth_be16_t checksum; // Checksum
		seth_be16_t urgent;	  // Urgent pointer
} tcp_header_t;

template <typename T>
concept TCPAllowedType = std::is_same_v<T, IPv4Packet> || std::is_same_v<T, IPv6Packet>;

template <TCPAllowedType T> class TCPPacketTmpl : public T {
	protected:
		seth_be16_t src_port;
		seth_be16_t dst_port;
		seth_be32_t sequence;
		seth_be32_t ack;
		uint8_t offset; // Offset in 32-bit words, minimum 5

		bool opt_cwr;
		bool opt_ece;
		bool opt_urg;
		bool opt_ack;
		bool opt_psh;
		bool opt_rst;
		bool opt_syn;
		bool opt_fin;

		seth_be16_t window;
		seth_be16_t checksum;
		seth_be16_t urgent;

	private:
		void _clear();

	public:
		TCPPacketTmpl();
		TCPPacketTmpl(const std::vector<uint8_t> &data);

		~TCPPacketTmpl();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		uint16_t getSrcPort() const;
		void setSrcPort(uint16_t newSrcPort);

		uint16_t getDstPort() const;
		void setDstPort(uint16_t newDstPort);

		uint32_t getSequence() const;
		void setSequence(uint32_t newSequence);

		uint32_t getAck() const;
		void setAck(uint32_t newAck);

		uint8_t getOffset() const;
		void setOffset(uint8_t newOffset);

		bool getOptCWR() const;
		void setOptCWR(bool newOptCWR);

		bool getOptECE() const;
		void setOptECE(bool newOptECE);

		bool getOptURG() const;
		void setOptURG(bool newOptURG);

		bool getOptACK() const;
		void setOptACK(bool newOptAck);

		bool getOptPSH() const;
		void setOptPSH(bool newOptPsh);

		bool getOptRST() const;
		void setOptRST(bool newOptRst);

		bool getOptSYN() const;
		void setOptSYN(bool newOptSyn);

		bool getOptFIN() const;
		void setOptFIN(bool newOptFin);

		uint16_t getWindow() const;
		void setWindow(uint16_t newWindow);

		uint16_t getChecksum() const;
		void setChecksum(uint16_t newChecksum);

		uint16_t getUrgent() const;
		void setUrgent(uint16_t newUrgent);

		uint16_t getHeaderOffset() const override;
		uint16_t getHeaderSize() const override;
		uint16_t getPacketSize() const override;

		std::string asText() const override;
		std::string asBinary() const override;
};

// Define types we plan to use
template class TCPPacketTmpl<IPv4Packet>;
template class TCPPacketTmpl<IPv6Packet>;

// Create class aliases
using TCPv4Packet = TCPPacketTmpl<IPv4Packet>;
using TCPv6Packet = TCPPacketTmpl<IPv6Packet>;
