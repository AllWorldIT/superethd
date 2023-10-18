#include "ICMPv4Packet.hpp"
#include "IPPacket.hpp"
#include "IPv4Packet.hpp"

void ICMPv4Packet::_clear() {
	setProtocol(SETH_PACKET_IP_PROTOCOL_ICMP4);

	type = 0;
	code = 0;
	checksum = 0;
}

ICMPv4Packet::ICMPv4Packet() : IPv4Packet() { _clear(); }

ICMPv4Packet::ICMPv4Packet(const std::vector<uint8_t> &data) : IPv4Packet(data) {}

ICMPv4Packet::~ICMPv4Packet() {  }

void ICMPv4Packet::clear() {
	IPv4Packet::clear();
	_clear();
}

void ICMPv4Packet::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	IPv4Packet::parse(data);
}

uint8_t ICMPv4Packet::getType() const { return type; }
void ICMPv4Packet::setType(uint8_t newType) { type = newType; }

uint8_t ICMPv4Packet::getCode() const { return code; }
void ICMPv4Packet::setCode(uint8_t newCode) { code = newCode; }

uint8_t ICMPv4Packet::getChecksum() const { return seth_be_to_cpu_16(checksum); }

uint16_t ICMPv4Packet::getHeaderOffset() const { return IPv4Packet::getHeaderOffset() + IPv4Packet::getHeaderSize(); }
uint16_t ICMPv4Packet::getHeaderSize() const { return sizeof(icmp_header_t); }

uint16_t ICMPv4Packet::getPacketSize() const {
	// TODO
	return getHeaderOffset() + getHeaderSize() + getPayloadSize();
};

std::string ICMPv4Packet::asText() const {
	std::ostringstream oss;

	oss << IPv4Packet::asText() << std::endl;

	oss << "==> ICMPv4 Packet" << std::endl;

	oss << std::format("*Header Offset : {}", getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size   : {}", getHeaderSize()) << std::endl;

	oss << std::format("Type           : {}", getType()) << std::endl;
	oss << std::format("Code           : {}", getCode()) << std::endl;
	oss << std::format("Checksum       : {}", getChecksum()) << std::endl;

	return oss.str();
}

std::string ICMPv4Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);
	return oss.str();
}
