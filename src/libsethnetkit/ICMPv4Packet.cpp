#include "ICMPv4Packet.hpp"

ICMPv4Packet::ICMPv4Packet(const std::vector<uint8_t> &data) : IPv4Packet(data) {}

void ICMPv4Packet::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	IPv4Packet::parse(data);
}

uint8_t ICMPv4Packet::getType() const { return type; }
void ICMPv4Packet::setType(uint8_t newType) { type = newType; }

uint8_t ICMPv4Packet::getCode() const { return code; }
void ICMPv4Packet::setCode(uint8_t newCode) { code = newCode; }

uint8_t ICMPv4Packet::getChecksum() const { return seth_be_to_cpu_16(checksum); }

std::string ICMPv4Packet::asText() const {
	std::ostringstream oss;

	oss << Packet::asText() << std::endl;

	oss << "==> ICMPv4 Packet" << std::endl;

	oss << std::format("Type          : ", getType()) << std::endl;
	oss << std::format("Code          : ", getCode()) << std::endl;
	oss << std::format("Checksum      : ", getChecksum()) << std::endl;
	oss << std::format("Version       : ", getVersion()) << std::endl;

	return oss.str();
}

std::string ICMPv4Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);
	return oss.str();
}
