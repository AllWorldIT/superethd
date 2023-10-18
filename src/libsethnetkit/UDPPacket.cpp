/*
 * UDP packet handling.
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

#include "UDPPacket.hpp"
#include "exceptions.hpp"
#include <type_traits>

template <UDPAllowedType T> void UDPPacketTmpl<T>::_clear() {

	// Switch out the method used to set the Layer 4 protocol
	if constexpr (std::is_same<IPv4Packet, T>::value) {
		DEBUG_PRINT("Protocol is IPv4Packet");
		T::setProtocol(SETH_PACKET_IP_PROTOCOL_UDP);
	} else if constexpr (std::is_same<IPv6Packet, T>::value) {
		DEBUG_PRINT("Protocol is IPv6Packet");
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
	_clear();
}

template <UDPAllowedType T> void UDPPacketTmpl<T>::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	T::parse(data);
}

template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getSrcPort() const { return seth_be_to_cpu_16(src_port); }
template <UDPAllowedType T> void UDPPacketTmpl<T>::setSrcPort(uint16_t newSrcPort) { src_port = seth_cpu_to_be_16(newSrcPort); }

template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getDstPort() const { return seth_be_to_cpu_16(dst_port); }
template <UDPAllowedType T> void UDPPacketTmpl<T>::setDstPort(uint16_t newDstPort) { dst_port = seth_cpu_to_be_16(newDstPort); }

template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getHeaderOffset() const { return T::getHeaderOffset() + T::getHeaderSize(); }
template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getHeaderSize() const { return sizeof(udp_header_t); }

template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getPacketSize() const {
	// TODO
	return getHeaderOffset() + getLengthLayer4();
}

template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getLengthLayer4() const { return getHeaderSize() + T::getPayloadSize(); }

// TODO: Checksum
template <UDPAllowedType T> uint16_t UDPPacketTmpl<T>::getChecksum() const { return seth_be_to_cpu_16(checksum); }

template <UDPAllowedType T> std::string UDPPacketTmpl<T>::asText() const {
	std::ostringstream oss;

	oss << T::asText() << std::endl;

	oss << "==> UDP Packet" << std::endl;

	oss << std::format("*Header Offset : {}", getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size   : {}", getHeaderSize()) << std::endl;

	oss << std::format("Source Port    : {}", getSrcPort()) << std::endl;
	oss << std::format("Dest. Port     : {}", getDstPort()) << std::endl;
	oss << std::format("Checksum       : {}", getChecksum()) << std::endl;

	oss << std::format("Length         : {}", getLengthLayer4()) << std::endl;

	return oss.str();
}

template <UDPAllowedType T> std::string UDPPacketTmpl<T>::asBinary() const {
	std::ostringstream oss(std::ios::binary);
	DEBUG_PRINT();
	oss << T::asBinary();

	udp_header_t header;

	header.src_port = src_port;
	header.dst_port = dst_port;
	header.length = seth_cpu_to_be_16(getLengthLayer4());
	// FIXME
	header.checksum = 0;

	oss.write(reinterpret_cast<const char *>(&header), sizeof(udp_header_t));
	oss.write(reinterpret_cast<const char *>(T::getPayloadPointer()), T::getPayloadSize());

	return oss.str();
}
