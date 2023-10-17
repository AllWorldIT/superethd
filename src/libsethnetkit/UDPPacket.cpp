#include "UDPPacket.hpp"
#include <type_traits>

template <typename T> void UDPPacketTmpl<T>::_clear() {
	src_port = 0;
	dst_port = 0;
	length = 0;
	checksum = 0;
}

template <typename T> UDPPacketTmpl<T>::UDPPacketTmpl() : T() { _clear(); }

template <typename T> UDPPacketTmpl<T>::UDPPacketTmpl(const std::vector<uint8_t> &data) : T(data) {}

template <typename T> UDPPacketTmpl<T>::~UDPPacketTmpl() { DEBUG_PRINT("Destruction!"); }

template <typename T> void UDPPacketTmpl<T>::clear() {
	T::clear();
	_clear();
}

template <typename T> void UDPPacketTmpl<T>::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	T::parse(data);
}

template <typename T> uint16_t UDPPacketTmpl<T>::getSrcPort() const { return seth_be_to_cpu_16(src_port); }
template <typename T> void UDPPacketTmpl<T>::setSrcPort(uint16_t newSrcPort) { src_port = seth_cpu_to_be_16(newSrcPort); }

template <typename T> uint16_t UDPPacketTmpl<T>::getDstPort() const { return seth_be_to_cpu_16(dst_port); }
template <typename T> void UDPPacketTmpl<T>::setDstPort(uint16_t newDstPort) { dst_port = seth_cpu_to_be_16(newDstPort); }

template <typename T> uint16_t UDPPacketTmpl<T>::getLength() const { return seth_be_to_cpu_16(length); };

template <typename T> uint16_t UDPPacketTmpl<T>::getChecksum() const { return seth_be_to_cpu_16(checksum); }

template <typename T> std::string UDPPacketTmpl<T>::asText() const {
	std::ostringstream oss;

	oss << Packet::asText() << std::endl;

	oss << "==> UDP Packet" << std::endl;

	oss << std::format("Source Port   : ", getSrcPort()) << std::endl;
	oss << std::format("Dest. Port    : ", getDstPort()) << std::endl;
	oss << std::format("Length        : ", getLength()) << std::endl;
	oss << std::format("Checksum      : ", getChecksum()) << std::endl;

	return oss.str();
}

template <typename T> std::string UDPPacketTmpl<T>::asBinary() const {
	std::ostringstream oss(std::ios::binary);

	oss << T::asBinary();

	return oss.str();
}
