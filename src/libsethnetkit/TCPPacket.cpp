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

#include "TCPPacket.hpp"
#include "IPPacket.hpp"
#include "IPv4Packet.hpp"
#include "IPv6Packet.hpp"

template <TCPAllowedType T> void TCPPacketTmpl<T>::_clear() {

	// Switch out the method used to set the Layer 4 protocol
	if constexpr (std::is_same<IPv4Packet, T>::value) {
		T::setProtocol(SETH_PACKET_IP_PROTOCOL_TCP);
	} else if constexpr (std::is_same<IPv6Packet, T>::value) {
		T::setNextHeader(SETH_PACKET_IP_PROTOCOL_TCP);
	}

	src_port = 0;
	dst_port = 0;
	sequence = 0;
	ack = 0;
	offset = 0;

	opt_cwr = false;
	opt_ece = false;
	opt_urg = false;
	opt_ack = false;
	opt_psh = false;
	opt_rst = false;
	opt_syn = false;
	opt_fin = false;

	window = 0;
	checksum = 0;
	urgent = 0;
}

template <TCPAllowedType T> TCPPacketTmpl<T>::TCPPacketTmpl() : T() { _clear(); }

template <TCPAllowedType T> TCPPacketTmpl<T>::TCPPacketTmpl(const std::vector<uint8_t> &data) : T(data) {}

template <TCPAllowedType T> TCPPacketTmpl<T>::~TCPPacketTmpl() {}

template <TCPAllowedType T> void TCPPacketTmpl<T>::clear() {
	T::clear();
	_clear();
}

template <TCPAllowedType T> void TCPPacketTmpl<T>::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	T::parse(data);
}

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getSrcPort() const { return seth_be_to_cpu_16(src_port); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setSrcPort(uint16_t newSrcPort) { src_port = seth_cpu_to_be_16(newSrcPort); }

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getDstPort() const { return seth_be_to_cpu_16(dst_port); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setDstPort(uint16_t newDstPort) { dst_port = seth_cpu_to_be_16(newDstPort); }

template <TCPAllowedType T> uint32_t TCPPacketTmpl<T>::getSequence() const { return seth_be_to_cpu_32(sequence); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setSequence(uint32_t newSequence) { sequence = seth_be_to_cpu_32(newSequence); }

template <TCPAllowedType T> uint32_t TCPPacketTmpl<T>::getAck() const { return seth_be_to_cpu_32(ack); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setAck(uint32_t newAck) { ack = seth_be_to_cpu_32(newAck); }

template <TCPAllowedType T> uint8_t TCPPacketTmpl<T>::getOffset() const { return offset; }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setOffset(uint8_t newOffset) { ack = newOffset; }

template <TCPAllowedType T> bool TCPPacketTmpl<T>::getOptCWR() const { return opt_cwr; }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setOptCWR(bool newOptCWR) { opt_cwr = newOptCWR; }

template <TCPAllowedType T> bool TCPPacketTmpl<T>::getOptECE() const { return opt_ece; }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setOptECE(bool newOptECE) { opt_ece = newOptECE; }

template <TCPAllowedType T> bool TCPPacketTmpl<T>::getOptURG() const { return opt_urg; }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setOptURG(bool newOptURG) { opt_urg = newOptURG; }

template <TCPAllowedType T> bool TCPPacketTmpl<T>::getOptACK() const { return opt_ack; }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setOptACK(bool newOptAck) { opt_ack = newOptAck; }

template <TCPAllowedType T> bool TCPPacketTmpl<T>::getOptPSH() const { return opt_psh; }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setOptPSH(bool newOptPsh) { opt_psh = newOptPsh; }

template <TCPAllowedType T> bool TCPPacketTmpl<T>::getOptRST() const { return opt_rst; }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setOptRST(bool newOptRst) { opt_rst = newOptRst; }

template <TCPAllowedType T> bool TCPPacketTmpl<T>::getOptSYN() const { return opt_syn; }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setOptSYN(bool newOptSyn) { opt_syn = newOptSyn; }

template <TCPAllowedType T> bool TCPPacketTmpl<T>::getOptFIN() const { return opt_fin; }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setOptFIN(bool newOptFin) { opt_fin = newOptFin; }

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getWindow() const { return seth_be_to_cpu_32(window); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setWindow(uint16_t newWindow) { ack = seth_be_to_cpu_32(newWindow); }

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getChecksum() const { return seth_be_to_cpu_32(checksum); }

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getUrgent() const { return seth_be_to_cpu_32(urgent); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setUrgent(uint16_t newUrgent) { ack = seth_be_to_cpu_32(newUrgent); }

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getHeaderOffset() const { return T::getHeaderOffset() + T::getHeaderSize(); }
template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getHeaderSize() const { return sizeof(tcp_header_t); }

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getPacketSize() const {
	// TODO
	return getHeaderOffset() + getHeaderSize() + T::getPayloadSize();
};

template <TCPAllowedType T> std::string TCPPacketTmpl<T>::asText() const {
	std::ostringstream oss;

	oss << T::asText() << std::endl;

	oss << "==> TCP Packet" << std::endl;

	oss << std::format("*Header Offset : {}", getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size   : {}", getHeaderSize()) << std::endl;

	oss << std::format("Source Port    : {}", getSrcPort()) << std::endl;
	oss << std::format("Dest. Port     : {}", getDstPort()) << std::endl;
	oss << std::format("Sequence       : {}", getSequence()) << std::endl;
	oss << std::format("Ack            : {}", getAck()) << std::endl;
	oss << std::format("IHL            : {}", getOffset()) << std::endl;
	oss << std::format("Options        : ") << std::endl;
	oss << "           FIN: " << getOptFIN() << std::endl;
	oss << "           SYN: " << getOptSYN() << std::endl;
	oss << "           RST: " << getOptRST() << std::endl;
	oss << "           PSH: " << getOptPSH() << std::endl;
	oss << "           ACK: " << getOptACK() << std::endl;
	oss << "           URG: " << getOptURG() << std::endl;
	oss << "           ECE: " << getOptECE() << std::endl;
	oss << std::format("Window         : {}", getWindow()) << std::endl;
	oss << std::format("Checksum       : {}", getChecksum()) << std::endl;
	oss << std::format("Urgent         : {}", getUrgent()) << std::endl;

	return oss.str();
}

template <TCPAllowedType T> std::string TCPPacketTmpl<T>::asBinary() const {
	std::ostringstream oss(std::ios::binary);

	oss << T::asBinary();

	return oss.str();
}
