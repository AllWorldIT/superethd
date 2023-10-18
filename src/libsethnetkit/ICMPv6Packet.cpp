#include "ICMPv6Packet.hpp"
#include "IPPacket.hpp"
#include "IPv6Packet.hpp"

void ICMPv6Packet::_clear() {
	setNextHeader(SETH_PACKET_IP_PROTOCOL_ICMP6);

	type = 0;
	code = 0;
	checksum = 0;
}

ICMPv6Packet::ICMPv6Packet() : IPv6Packet() { _clear(); }

ICMPv6Packet::ICMPv6Packet(const std::vector<uint8_t> &data) : IPv6Packet(data) {}

ICMPv6Packet::~ICMPv6Packet() {}

void ICMPv6Packet::clear() {
	IPv6Packet::clear();
	_clear();
}

void ICMPv6Packet::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	IPv6Packet::parse(data);
}

uint8_t ICMPv6Packet::getType() const { return type; }
void ICMPv6Packet::setType(uint8_t newType) { type = newType; }

uint8_t ICMPv6Packet::getCode() const { return code; }
void ICMPv6Packet::setCode(uint8_t newCode) { code = newCode; }

uint8_t ICMPv6Packet::getChecksum() const { return seth_be_to_cpu_16(checksum); }

uint16_t ICMPv6Packet::getHeaderOffset() const { return IPv6Packet::getHeaderOffset() + IPv6Packet::getHeaderSize(); }
uint16_t ICMPv6Packet::getHeaderSize() const { return sizeof(icmp6_header_t); }

uint16_t ICMPv6Packet::getPacketSize() const {
	// TODO
	return getHeaderOffset() + getHeaderSize() + getPayloadSize();
};

std::string ICMPv6Packet::asText() const {
	std::ostringstream oss;

	oss << IPv6Packet::asText() << std::endl;

	oss << "==> ICMPv6 Packet" << std::endl;

	oss << std::format("*Header Offset : {}", getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size   : {}", getHeaderSize()) << std::endl;

	oss << std::format("Type          : ", getType()) << std::endl;
	oss << std::format("Code          : ", getCode()) << std::endl;
	oss << std::format("Checksum      : ", getChecksum()) << std::endl;

	return oss.str();
}

std::string ICMPv6Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);
	return oss.str();
}
