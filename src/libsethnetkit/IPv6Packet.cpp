#include "IPv6Packet.hpp"

void IPv6Packet::_clear() {
	setVersion(SETH_PACKET_IP_VERSION_IPV6);

	traffic_class = 0;
	flow_label = 0;
	payload_length = 0;
	next_header = 0;
	hop_limit = 0;
	// Clear IP's
	for (auto &element : dst_addr) {
		element = 0;
	}
	for (auto &element : src_addr) {
		element = 0;
	}
}

IPv6Packet::IPv6Packet() : IPPacket() {
	_clear();
}

IPv6Packet::IPv6Packet(const std::vector<uint8_t> &data) : IPPacket(data) { setVersion(SETH_PACKET_IP_VERSION_IPV6); }

IPv6Packet::~IPv6Packet() { DEBUG_PRINT("Destruction!"); }

void IPv6Packet::clear() {
	IPPacket::clear();
	_clear();
}

void IPv6Packet::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	IPPacket::parse(data);
	setVersion(SETH_PACKET_IP_VERSION_IPV6);
}

uint8_t IPv6Packet::getTrafficClass() const { return traffic_class; }
void IPv6Packet::setTrafficClass(uint8_t newTrafficClass) { traffic_class = newTrafficClass; }

uint8_t IPv6Packet::getFlowLabel() const { return flow_label; }
void IPv6Packet::setFlowLabel(uint8_t newFlowLabel) { flow_label = newFlowLabel; }

seth_be16_t IPv6Packet::getPayloadLength() const { return seth_be_to_cpu_16(payload_length); }

uint8_t IPv6Packet::getNextHeader() const { return next_header; }

uint8_t IPv6Packet::getHopLimit() const { return hop_limit; }
void IPv6Packet::setHopLimit(uint8_t newHopLimit) { hop_limit = newHopLimit; }

std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> IPv6Packet::getDstAddr() const { return dst_addr; }
void IPv6Packet::setDstAddr(std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> newDstAddr) { dst_addr = newDstAddr; }

std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> IPv6Packet::getSrcAddr() const { return src_addr; }
void IPv6Packet::setSrcAddr(std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> newSrcAddr) { src_addr = newSrcAddr; }

std::string IPv6Packet::asText() const {
	std::ostringstream oss;

	oss << Packet::asText() << std::endl;

	oss << "==> IPv6" << std::endl;

	oss << std::format("Traffic Class : ", getTrafficClass()) << std::endl;
	oss << std::format("Flow Label    : ", getFlowLabel()) << std::endl;
	oss << std::format("Payload Length: ", getPayloadLength()) << std::endl;
	oss << std::format("Next Header   : ", getNextHeader()) << std::endl;
	oss << std::format("Hop Limit     : ", getHopLimit()) << std::endl;

	std::array<uint8_t, SETH_PACKET_IPV6_IP_LEN> ip;

	ip = getDstAddr();
	oss << std::format("Destination IP: "
					   "{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}",
					   ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7], ip[8], ip[9], ip[10], ip[11], ip[12], ip[13], ip[14],
					   ip[15])
		<< std::endl;

	ip = getSrcAddr();
	oss << std::format("Source IP     : "
					   "{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}:{:02X}{:02X}",
					   ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7], ip[8], ip[9], ip[10], ip[11], ip[12], ip[13], ip[14],
					   ip[15])
		<< std::endl;

	return oss.str();
}

std::string IPv6Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);
	return oss.str();
}