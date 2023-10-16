#include "IPPacket.hpp"

IPPacket::IPPacket(const std::vector<uint8_t> &data) : EthernetPacket(data) {}

void IPPacket::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	EthernetPacket::parse(data);
}

uint8_t IPPacket::getVersion() const { return version; }
void IPPacket::setVersion(uint8_t newVersion) { version = newVersion; }

std::string IPPacket::asText() const {
	std::ostringstream oss;

	oss << Packet::asText() << std::endl;

	oss << "==> IP" << std::endl;

	oss << std::format("Version       : ", getVersion()) << std::endl;

	return oss.str();
}

std::string IPPacket::asBinary() const { return EthernetPacket::asBinary(); }
