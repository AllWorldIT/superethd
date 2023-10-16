#include "TCPv4Packet.hpp"

TCPv4Packet::TCPv4Packet(const std::vector<uint8_t> &data) : IPv4Packet(data) {}

void TCPv4Packet::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	IPv4Packet::parse(data);
}

uint16_t TCPv4Packet::getSrcPort() const { return seth_be_to_cpu_16(src_port); }
void TCPv4Packet::setSrcPort(uint16_t newSrcPort) { src_port = seth_cpu_to_be_16(newSrcPort); }

uint16_t TCPv4Packet::getDstPort() const { return seth_be_to_cpu_16(dst_port); }
void TCPv4Packet::setDstPort(uint16_t newDstPort) { dst_port = seth_cpu_to_be_16(newDstPort); }

uint32_t TCPv4Packet::getSequence() const { return seth_be_to_cpu_32(sequence); }
void TCPv4Packet::setSequence(uint32_t newSequence) { sequence = seth_be_to_cpu_32(newSequence); }

uint32_t TCPv4Packet::getAck() const { return seth_be_to_cpu_32(ack); }
void TCPv4Packet::setAck(uint32_t newAck) { ack = seth_be_to_cpu_32(newAck); }

uint8_t TCPv4Packet::getOffset() const { return offset; }
void TCPv4Packet::setOffset(uint8_t newOffset) { ack = newOffset; }



bool TCPv4Packet::getOptCWR() const { return opt_cwr; }
void TCPv4Packet::setOptCWR(bool newOptCWR) {opt_cwr = newOptCWR; }

bool TCPv4Packet::getOptECE() const {return opt_ece; }
void TCPv4Packet::setOptECE(bool newOptECE) {opt_ece = newOptECE; }

bool TCPv4Packet::getOptURG() const {return opt_urg; }
void TCPv4Packet::setOptURG(bool newOptURG) {opt_urg = newOptURG; }

bool TCPv4Packet::getOptACK() const {return opt_ack; }
void TCPv4Packet::setOptACK(bool newOptAck) {opt_ack = newOptAck; }

bool TCPv4Packet::getOptPSH() const {return opt_psh; }
void TCPv4Packet::setOptPSH(bool newOptPsh) {opt_psh = newOptPsh; }

bool TCPv4Packet::getOptRST() const {return opt_rst; }
void TCPv4Packet::setOptRST(bool newOptRst) {opt_rst = newOptRst; }

bool TCPv4Packet::getOptSYN() const {return opt_syn; }
void TCPv4Packet::setOptSYN(bool newOptSyn) {opt_syn = newOptSyn; }

bool TCPv4Packet::getOptFIN() const {return opt_fin; }
void TCPv4Packet::setOptFIN(bool newOptFin) {opt_fin = newOptFin; }



uint16_t TCPv4Packet::getWindow() const { return seth_be_to_cpu_32(window); }
void TCPv4Packet::setWindow(uint16_t newWindow) { ack = seth_be_to_cpu_32(newWindow); }

uint16_t TCPv4Packet::getChecksum() const { return seth_be_to_cpu_32(checksum); }

uint16_t TCPv4Packet::getUrgent() const { return seth_be_to_cpu_32(urgent); }
void TCPv4Packet::setUrgent(uint16_t newUrgent) { ack = seth_be_to_cpu_32(newUrgent); }

std::string TCPv4Packet::asText() const {
	std::ostringstream oss;

	oss << Packet::asText() << std::endl;

	oss << "==> TCPv4 Packet" << std::endl;

	oss << std::format("Source Port   : ", getSrcPort()) << std::endl;
	oss << std::format("Dest. Port    : ", getDstPort()) << std::endl;
	oss << std::format("Sequence      : ", getSequence()) << std::endl;
	oss << std::format("Ack           : ", getAck()) << std::endl;
	oss << std::format("IHL           : ", getOffset()) << std::endl;
	oss << std::format("Options       : ") << std::endl;
	oss << std::format("           FIN: ") << getOptFIN() << std::endl;
	oss << std::format("           SYN: ") << getOptSYN() << std::endl;
	oss << std::format("           RST: ") << getOptRST() << std::endl;
	oss << std::format("           PSH: ") << getOptPSH() << std::endl;
	oss << std::format("           ACK: ") << getOptACK() << std::endl;
	oss << std::format("           URG: ") << getOptURG() << std::endl;
	oss << std::format("           ECE: ") << getOptECE() << std::endl;
	oss << std::format("Window        : ", getWindow()) << std::endl;
	oss << std::format("Checksum      : ", getChecksum()) << std::endl;
	oss << std::format("Urgent        : ", getUrgent()) << std::endl;

	return oss.str();
}

std::string TCPv4Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);
	return oss.str();
}