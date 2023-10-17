#include "TCPPacket.hpp"

template <typename T> void TCPPacketTmpl<T>::_clear() {
	src_port = 0;
	dst_port = 0;
	sequence = 0;
	ack = 0;
	offset = 0;

	opt_cwr = false;
	opt_ece = false;
	opt_urg = false;
	opt_ack = false;
	opt_psh = false;
	opt_rst = false;
	opt_syn = false;
	opt_fin = false;

	window = 0;
	checksum = 0;
	urgent = 0;
}

template <typename T> TCPPacketTmpl<T>::TCPPacketTmpl() : T() {
	_clear();
}

template <typename T> TCPPacketTmpl<T>::TCPPacketTmpl(const std::vector<uint8_t> &data) : T(data) {}

template <typename T> TCPPacketTmpl<T>::~TCPPacketTmpl() { DEBUG_PRINT("Destruction!"); }

template <typename T> void TCPPacketTmpl<T>::clear() {
	T::clear();
	_clear();
}

template <typename T> void TCPPacketTmpl<T>::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	T::parse(data);
}

template <typename T> uint16_t TCPPacketTmpl<T>::getSrcPort() const { return seth_be_to_cpu_16(src_port); }
template <typename T> void TCPPacketTmpl<T>::setSrcPort(uint16_t newSrcPort) { src_port = seth_cpu_to_be_16(newSrcPort); }

template <typename T> uint16_t TCPPacketTmpl<T>::getDstPort() const { return seth_be_to_cpu_16(dst_port); }
template <typename T> void TCPPacketTmpl<T>::setDstPort(uint16_t newDstPort) { dst_port = seth_cpu_to_be_16(newDstPort); }

template <typename T> uint32_t TCPPacketTmpl<T>::getSequence() const { return seth_be_to_cpu_32(sequence); }
template <typename T> void TCPPacketTmpl<T>::setSequence(uint32_t newSequence) { sequence = seth_be_to_cpu_32(newSequence); }

template <typename T> uint32_t TCPPacketTmpl<T>::getAck() const { return seth_be_to_cpu_32(ack); }
template <typename T> void TCPPacketTmpl<T>::setAck(uint32_t newAck) { ack = seth_be_to_cpu_32(newAck); }

template <typename T> uint8_t TCPPacketTmpl<T>::getOffset() const { return offset; }
template <typename T> void TCPPacketTmpl<T>::setOffset(uint8_t newOffset) { ack = newOffset; }

template <typename T> bool TCPPacketTmpl<T>::getOptCWR() const { return opt_cwr; }
template <typename T> void TCPPacketTmpl<T>::setOptCWR(bool newOptCWR) { opt_cwr = newOptCWR; }

template <typename T> bool TCPPacketTmpl<T>::getOptECE() const { return opt_ece; }
template <typename T> void TCPPacketTmpl<T>::setOptECE(bool newOptECE) { opt_ece = newOptECE; }

template <typename T> bool TCPPacketTmpl<T>::getOptURG() const { return opt_urg; }
template <typename T> void TCPPacketTmpl<T>::setOptURG(bool newOptURG) { opt_urg = newOptURG; }

template <typename T> bool TCPPacketTmpl<T>::getOptACK() const { return opt_ack; }
template <typename T> void TCPPacketTmpl<T>::setOptACK(bool newOptAck) { opt_ack = newOptAck; }

template <typename T> bool TCPPacketTmpl<T>::getOptPSH() const { return opt_psh; }
template <typename T> void TCPPacketTmpl<T>::setOptPSH(bool newOptPsh) { opt_psh = newOptPsh; }

template <typename T> bool TCPPacketTmpl<T>::getOptRST() const { return opt_rst; }
template <typename T> void TCPPacketTmpl<T>::setOptRST(bool newOptRst) { opt_rst = newOptRst; }

template <typename T> bool TCPPacketTmpl<T>::getOptSYN() const { return opt_syn; }
template <typename T> void TCPPacketTmpl<T>::setOptSYN(bool newOptSyn) { opt_syn = newOptSyn; }

template <typename T> bool TCPPacketTmpl<T>::getOptFIN() const { return opt_fin; }
template <typename T> void TCPPacketTmpl<T>::setOptFIN(bool newOptFin) { opt_fin = newOptFin; }

template <typename T> uint16_t TCPPacketTmpl<T>::getWindow() const { return seth_be_to_cpu_32(window); }
template <typename T> void TCPPacketTmpl<T>::setWindow(uint16_t newWindow) { ack = seth_be_to_cpu_32(newWindow); }

template <typename T> uint16_t TCPPacketTmpl<T>::getChecksum() const { return seth_be_to_cpu_32(checksum); }

template <typename T> uint16_t TCPPacketTmpl<T>::getUrgent() const { return seth_be_to_cpu_32(urgent); }
template <typename T> void TCPPacketTmpl<T>::setUrgent(uint16_t newUrgent) { ack = seth_be_to_cpu_32(newUrgent); }

template <typename T> std::string TCPPacketTmpl<T>::asText() const {
	std::ostringstream oss;

	oss << T::asText() << std::endl;

	oss << "==> TCP Packet" << std::endl;

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

template <typename T> std::string TCPPacketTmpl<T>::asBinary() const {
	std::ostringstream oss(std::ios::binary);

	oss << T::asBinary();

	return oss.str();
}
