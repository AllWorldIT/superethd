/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "UDPPacket.hpp"
#include "checksum.hpp"
#include "exceptions.hpp"
#include <type_traits>

template <UDPAllowedType T> void UDPPacketTmpl<T>::_clear() {

	// Switch out the method used to set the Layer 4 protocol
	if constexpr (std::is_same<IPv4Packet, T>::value) {
		T::setProtocol(SETH_PACKET_IP_PROTOCOL_UDP);
	} else if constexpr (std::is_same<IPv6Packet, T>::value) {
		T::setNextHeader(SETH_PACKET_IP_PROTOCOL_UDP);
	} else {
		throw PacketNotSupportedEception("Unknown packet");
	}

	src_port = 0;
	dst_port = 0;
	checksum = 0;
}

template <UDPAllowedType T> UDPPacketTmpl<T>::UDPPacketTmpl() : T() { _clear(); }

template <UDPAllowedType T> UDPPacketTmpl<T>::UDPPacketTmpl(const std::vector<uint8_t> &data) : T(data) {}

template <UDPAllowedType T> UDPPacketTmpl<T>::~UDPPacketTmpl() {}

template <UDPAllowedType T> void UDPPacketTmpl<T>::clear() {
	T::clear();
	UDPPacketTmpl<T>::_clear();
}

template <UDPAllowedType T> void UDPPacketTmpl<T>::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	T::parse(data);
}

template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getSrcPort() const { return seth_be_to_cpu_16(src_port); }
template <UDPAllowedType T> void UDPPacketTmpl<T>::setSrcPort(uint16_t newSrcPort) { src_port = seth_cpu_to_be_16(newSrcPort); }

template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getDstPort() const { return seth_be_to_cpu_16(dst_port); }
template <UDPAllowedType T> void UDPPacketTmpl<T>::setDstPort(uint16_t newDstPort) { dst_port = seth_cpu_to_be_16(newDstPort); }

template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getChecksumLayer4() const {
	// Populate UDP header to add onto the layer 3 pseudo header
	udp_header_t header;

	header.src_port = src_port;
	header.dst_port = dst_port;
	header.length = seth_cpu_to_be_16(getLayer4Size());
	header.checksum = 0;

	// Grab the layer3 checksum
	uint32_t partial_checksum = T::getPseudoChecksumLayer3(getLayer4Size());
	// Add the UDP packet header
	partial_checksum = compute_checksum_partial((uint8_t *)&header, sizeof(udp_header_t), partial_checksum);
	// Add payload
	partial_checksum = compute_checksum_partial((uint8_t *)UDPPacketTmpl<T>::getPayloadPointer(),
												UDPPacketTmpl<T>::getPayloadSize(), partial_checksum);
	// Finalize checksum and return
	return compute_checksum_finalize(partial_checksum);
}

template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getHeaderOffset() const { return T::getHeaderOffset() + T::getHeaderSize(); }
template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getHeaderSize() const { return sizeof(udp_header_t); }
template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getHeaderSizeTotal() const {
	return T::getHeaderSizeTotal() + getHeaderSize();
}
template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getLayer4Size() const {
	return getHeaderSize() + UDPPacketTmpl<T>::getPayloadSize();
}

template <UDPAllowedType T> std::string UDPPacketTmpl<T>::asText() const {
	std::ostringstream oss;

	oss << T::asText() << std::endl;

	oss << "==> UDP Packet" << std::endl;

	oss << std::format("*Header Offset : {}", getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size   : {}", getHeaderSize()) << std::endl;

	oss << std::format("Source Port    : {}", getSrcPort()) << std::endl;
	oss << std::format("Dest. Port     : {}", getDstPort()) << std::endl;
	oss << std::format("Checksum       : {:04X}", getChecksumLayer4()) << std::endl;

	oss << std::format("Length         : {}", getLayer4Size()) << std::endl;

	return oss.str();
}

template <UDPAllowedType T> std::string UDPPacketTmpl<T>::asBinary() const {
	std::ostringstream oss(std::ios::binary);

	oss << T::asBinary();

	// Build header to dump into the stream
	udp_header_t header;

	header.src_port = src_port;
	header.dst_port = dst_port;
	header.length = seth_cpu_to_be_16(getLayer4Size());
	header.checksum = seth_cpu_to_be_16(getChecksumLayer4());

	// Write out header to stream
	oss.write(reinterpret_cast<const char *>(&header), sizeof(udp_header_t));
	// Write out payload to stream
	oss.write(reinterpret_cast<const char *>(UDPPacketTmpl<T>::getPayloadPointer()), UDPPacketTmpl<T>::getPayloadSize());

	return oss.str();
}
