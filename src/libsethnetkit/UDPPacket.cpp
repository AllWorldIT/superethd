#include "UDPPacket.hpp"
#include <type_traits>

template <UDPAllowedType T> void UDPPacketTmpl<T>::_clear() {

	// Switch out the method used to set the Layer 4 protocol
	if constexpr (std::is_same<IPv4Packet, T>::value) {
		T::setProtocol(SETH_PACKET_IP_PROTOCOL_TCP);
	} else if constexpr (std::is_same<IPv6Packet, T>::value) {
		T::setNextHeader(SETH_PACKET_IP_PROTOCOL_TCP);
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
	return getHeaderOffset() + getHeaderSize() + T::getPayloadSize();
};

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

	oss << std::format("Length         : {}", getPacketSize()) << std::endl;

	return oss.str();
}

template <UDPAllowedType T> std::string UDPPacketTmpl<T>::asBinary() const {
	std::ostringstream oss(std::ios::binary);

	oss << T::asBinary();

	return oss.str();
}
