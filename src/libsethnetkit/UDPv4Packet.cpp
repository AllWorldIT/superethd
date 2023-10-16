#include "UDPv4Packet.hpp"

UDPv4Packet::UDPv4Packet(const std::vector<uint8_t> &data) : IPv4Packet(data) {}

void UDPv4Packet::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	IPv4Packet::parse(data);
}

uint16_t UDPv4Packet::getSrcPort() const { return seth_be_to_cpu_16(src_port); }
void UDPv4Packet::setSrcPort(uint16_t newSrcPort) { src_port = seth_cpu_to_be_16(newSrcPort); }

uint16_t UDPv4Packet::getDstPort() const { return seth_be_to_cpu_16(dst_port); }
void UDPv4Packet::setDstPort(uint16_t newDstPort) { dst_port = seth_cpu_to_be_16(newDstPort); }

uint16_t UDPv4Packet::getLength() const { return seth_be_to_cpu_16(length); };

uint16_t UDPv4Packet::getChecksum() const { return seth_be_to_cpu_16(checksum); }

std::string UDPv4Packet::asText() const {
	std::ostringstream oss;

	oss << Packet::asText() << std::endl;

	oss << "==> UDPv4 Packet" << std::endl;

	oss << std::format("Source Port   : ", getSrcPort()) << std::endl;
	oss << std::format("Dest. Port    : ", getDstPort()) << std::endl;
	oss << std::format("Length        : ", getLength()) << std::endl;
	oss << std::format("Checksum      : ", getChecksum()) << std::endl;

	return oss.str();
}

std::string UDPv4Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);
	return oss.str();
}