#include "IPv4Packet.hpp"
#include "IPPacket.hpp"

void IPv4Packet::_clear() {
	setVersion(SETH_PACKET_IP_VERSION_IPV4);

	ihl = 0;
	dscp = 0;
	ecn = 0;
	total_length = 0;
	id = 0;
	frag_off = 0;
	ttl = 0;
	protocol = 0;
	checksum = 0;
	// Clear IP's
	for (auto &element : dst_addr) {
		element = 0;
	}
	for (auto &element : src_addr) {
		element = 0;
	}
}

IPv4Packet::IPv4Packet() : IPPacket() {
	DEBUG_PRINT("Construct");
	_clear();
}

IPv4Packet::IPv4Packet(const std::vector<uint8_t> &data) : IPPacket(data) { setVersion(SETH_PACKET_IP_VERSION_IPV4); }

IPv4Packet::~IPv4Packet() { DEBUG_PRINT("Destruction!"); }

void IPv4Packet::clear() {
	DEBUG_PRINT("start clear");
	IPPacket::clear();
	DEBUG_PRINT("end clear");
	_clear();
}

void IPv4Packet::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	IPPacket::parse(data);
	setVersion(SETH_PACKET_IP_VERSION_IPV4);
}

uint8_t IPv4Packet::getIHL() const { return ihl; }
void IPv4Packet::setIHL(uint8_t newIHL) { ihl = newIHL; }

uint8_t IPv4Packet::getDSCP() const { return dscp; }
void IPv4Packet::setDSCP(uint8_t newDSCP) { dscp = newDSCP; }

uint8_t IPv4Packet::getECN() const { return ecn; }
void IPv4Packet::setECN(uint8_t newECN) { ecn = newECN; }

uint16_t IPv4Packet::getTotalLength() const { return seth_be_to_cpu_16(total_length); }

uint16_t IPv4Packet::getId() const { return seth_be_to_cpu_16(id); }
void IPv4Packet::setId(uint16_t newId) { id = seth_cpu_to_be_16(newId); }

uint16_t IPv4Packet::getFragOff() const { return seth_be_to_cpu_16(frag_off); }
void IPv4Packet::setFragOff(uint16_t newFragOff) { frag_off = seth_cpu_to_be_16(newFragOff); }

uint8_t IPv4Packet::getTtl() const { return ttl; }
void IPv4Packet::setTtl(uint8_t newTTL) { ttl = newTTL; }

uint8_t IPv4Packet::getProtocol() const { return protocol; }
void IPv4Packet::setProtocol(uint8_t newProtocol) { protocol = newProtocol; }

uint16_t IPv4Packet::getChecksum() const { return seth_be_to_cpu_16(checksum); }

std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> IPv4Packet::getDstAddr() const { return dst_addr; }
void IPv4Packet::setDstAddr(std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> newDstAddr) { dst_addr = newDstAddr; }

std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> IPv4Packet::getSrcAddr() const { return src_addr; }
void IPv4Packet::setSrcAddr(std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> newSrcAddr) { src_addr = newSrcAddr; }

std::string IPv4Packet::asText() const {
	std::ostringstream oss;

	oss << IPPacket::asText() << std::endl;

	oss << "==> IPv4" << std::endl;

	oss << std::format("IP Header Len : {}", getIHL()) << std::endl;
	oss << std::format("DSCP          : {}", getDSCP()) << std::endl;
	oss << std::format("ECN           : {}", getECN()) << std::endl;
	oss << std::format("Total Length  : {}", getTotalLength()) << std::endl;
	oss << std::format("ID            : {}", getId()) << std::endl;
	oss << std::format("Frag Offset   : {}", getFragOff()) << std::endl;
	oss << std::format("TTL           : {}", getTtl()) << std::endl;
	oss << std::format("Protocol      : {}", getProtocol()) << std::endl;
	oss << std::format("Checksum      : {:04X}", getChecksum()) << std::endl;

	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> ip;

	ip = getDstAddr();
	oss << std::format("Destination IP: {}.{}.{}.{}", ip[0], ip[1], ip[2], ip[3]) << std::endl;

	ip = getSrcAddr();
	oss << std::format("Source IP     : {}.{}.{}.{}", ip[0], ip[1], ip[2], ip[3]) << std::endl;

	return oss.str();
}

std::string IPv4Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);

	oss << IPPacket::asBinary();

	return oss.str();
}
