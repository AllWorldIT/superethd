/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "tcp_packet.hpp"
#include "ipv4_packet.hpp"
#include "ipv6_packet.hpp"
#include "checksum.hpp"
#include <format>
#include <ostream>
#include <sstream>

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
	offset = 5;

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

template <TCPAllowedType T> TCPPacketTmpl<T>::TCPPacketTmpl(const std::vector<uint8_t> &data) : T(data) {
	_clear();
	parse(data);
}

template <TCPAllowedType T> TCPPacketTmpl<T>::~TCPPacketTmpl() {}

template <TCPAllowedType T> void TCPPacketTmpl<T>::clear() {
	T::clear();
	TCPPacketTmpl<T>::_clear();
}

template <TCPAllowedType T> void TCPPacketTmpl<T>::parse(const std::vector<uint8_t> &data) {}

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getSrcPort() const { return accl::be_to_cpu_16(src_port); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setSrcPort(uint16_t newSrcPort) { src_port = accl::cpu_to_be_16(newSrcPort); }

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getDstPort() const { return accl::be_to_cpu_16(dst_port); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setDstPort(uint16_t newDstPort) { dst_port = accl::cpu_to_be_16(newDstPort); }

template <TCPAllowedType T> uint32_t TCPPacketTmpl<T>::getSequence() const { return accl::be_to_cpu_32(sequence); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setSequence(uint32_t newSequence) { sequence = accl::be_to_cpu_32(newSequence); }

template <TCPAllowedType T> uint32_t TCPPacketTmpl<T>::getAck() const { return accl::be_to_cpu_32(ack); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setAck(uint32_t newAck) { ack = accl::be_to_cpu_32(newAck); }

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

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getWindow() const { return accl::be_to_cpu_32(window); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setWindow(uint16_t newWindow) { ack = accl::be_to_cpu_32(newWindow); }

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getUrgent() const { return accl::be_to_cpu_32(urgent); }
template <TCPAllowedType T> void TCPPacketTmpl<T>::setUrgent(uint16_t newUrgent) { ack = accl::be_to_cpu_32(newUrgent); }

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getChecksumLayer4() const {
	// Populate UDP header to add onto the layer 3 pseudo header
	tcp_header_t header;

	header.src_port = src_port;
	header.dst_port = dst_port;
	header.sequence = sequence;
	header.ack = ack;
	header.offset = offset;
	header.reserved = 0;
	header.options.cwr = opt_cwr;
	header.options.ece = opt_ece;
	header.options.urg = opt_urg;
	header.options.ack = opt_ack;
	header.options.psh = opt_psh;
	header.options.rst = opt_rst;
	header.options.syn = opt_syn;
	header.options.fin = opt_fin;

	header.window = window;
	header.urgent = urgent;

	header.checksum = 0;

	// Grab the layer3 checksum
	uint32_t partial_checksum = T::getPseudoChecksumLayer3(getLayer4Size());
	// Add the UDP packet header
	partial_checksum = compute_checksum_partial((uint8_t *)&header, sizeof(tcp_header_t), partial_checksum);
	// Add payload
	partial_checksum = compute_checksum_partial((uint8_t *)TCPPacketTmpl<T>::getPayloadPointer(),
												TCPPacketTmpl<T>::getPayloadSize(), partial_checksum);
	// Finalize checksum and return
	return compute_checksum_finalize(partial_checksum);
}

template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getHeaderOffset() const { return T::getHeaderOffset() + T::getHeaderSize(); }
template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getHeaderSize() const { return sizeof(tcp_header_t); }
template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getHeaderSizeTotal() const {
	return T::getHeaderSizeTotal() + getHeaderSize();
}
template <TCPAllowedType T> uint16_t TCPPacketTmpl<T>::getLayer4Size() const {
	return getHeaderSize() + TCPPacketTmpl<T>::getPayloadSize();
}
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
	oss << std::format("Offset         : {}", getOffset()) << std::endl;
	oss << std::format("Options        : ") << std::endl;
	oss << "           FIN: " << getOptFIN() << std::endl;
	oss << "           SYN: " << getOptSYN() << std::endl;
	oss << "           RST: " << getOptRST() << std::endl;
	oss << "           PSH: " << getOptPSH() << std::endl;
	oss << "           ACK: " << getOptACK() << std::endl;
	oss << "           URG: " << getOptURG() << std::endl;
	oss << "           ECE: " << getOptECE() << std::endl;
	oss << std::format("Window         : {}", getWindow()) << std::endl;
	oss << std::format("Checksum       : {:04X}", getChecksumLayer4()) << std::endl;
	oss << std::format("Urgent         : {}", getUrgent()) << std::endl;

	return oss.str();
}

template <TCPAllowedType T> std::string TCPPacketTmpl<T>::asBinary() const {
	std::ostringstream oss(std::ios::binary);

	oss << T::asBinary();

	// Build header to dump into the stream
	tcp_header_t header;

	header.src_port = src_port;
	header.dst_port = dst_port;
	header.sequence = sequence;
	header.ack = ack;
	header.offset = offset;
	header.reserved = 0;
	header.options.cwr = opt_cwr;
	header.options.ece = opt_ece;
	header.options.urg = opt_urg;
	header.options.ack = opt_ack;
	header.options.psh = opt_psh;
	header.options.rst = opt_rst;
	header.options.syn = opt_syn;
	header.options.fin = opt_fin;

	header.window = window;
	header.urgent = urgent;

	header.checksum = accl::cpu_to_be_16(getChecksumLayer4());

	// Write out header to stream
	oss.write(reinterpret_cast<const char *>(&header), sizeof(tcp_header_t));
	// Write out payload to stream
	oss.write(reinterpret_cast<const char *>(TCPPacketTmpl<T>::getPayloadPointer()), TCPPacketTmpl<T>::getPayloadSize());

	return oss.str();
}
