/*
 * IPv4 packet handling.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "IPv4Packet.hpp"
#include "IPPacket.hpp"
#include "checksum.hpp"

void IPv4Packet::_clear() {
	setEthertype(SETH_PACKET_ETHERTYPE_ETHERNET_IPV4);
	setVersion(SETH_PACKET_IP_VERSION_IPV4);

	ihl = 5;
	dscp = 0;
	ecn = 0;
	//	total_length = 0;
	id = 0;
	frag_off = 0;
	ttl = 64;
	protocol = 0;

	// Clear IP's
	for (auto &element : dst_addr) {
		element = 0;
	}
	for (auto &element : src_addr) {
		element = 0;
	}
}

IPv4Packet::IPv4Packet() : IPPacket() { _clear(); }

uint16_t IPv4Packet::_getIPv4Checksum() const {
	ipv4_header_t header;

	header.ihl = ihl;
	header.version = 4;
	header.dscp = dscp;
	header.ecn = ecn;
	header.total_length = seth_cpu_to_be_16(getLayer3Size());
	header.id = seth_cpu_to_be_16(id);
	header.frag_off = seth_cpu_to_be_16(frag_off);
	header.ttl = ttl;
	header.protocol = protocol;

	std::copy(src_addr.begin(), src_addr.end(), header.src_addr);
	std::copy(dst_addr.begin(), dst_addr.end(), header.dst_addr);


	header.checksum = 0;
	// TODO: the checksum should be calculated including optional headers
	// We should copy this header into a buffer
	// Then copy the optional headers into the buffer
	// Then do the checksum
	uint32_t partial_checksum = compute_checksum_partial((uint8_t *)&header, sizeof(ipv4_header_t), 0);
	uint16_t checksum = compute_checksum_finalize(partial_checksum);

	return checksum;
}

IPv4Packet::IPv4Packet(const std::vector<uint8_t> &data) : IPPacket(data) { _clear(); }

IPv4Packet::~IPv4Packet() {}

void IPv4Packet::clear() {
	IPPacket::clear();
	IPv4Packet::_clear();
}

void IPv4Packet::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	IPPacket::parse(data);
	setVersion(SETH_PACKET_IP_VERSION_IPV4);
}

uint8_t IPv4Packet::getIHL() const { return ihl; }

uint8_t IPv4Packet::getDSCP() const { return dscp; }
void IPv4Packet::setDSCP(uint8_t newDSCP) { dscp = newDSCP; }

uint8_t IPv4Packet::getECN() const { return ecn; }
void IPv4Packet::setECN(uint8_t newECN) { ecn = newECN; }

// uint16_t IPv4Packet::getTotalLength() const { return seth_be_to_cpu_16(total_length); }
// uint16_t IPv4Packet::getTotalLength() const { return (ihl * 4); }

uint16_t IPv4Packet::getId() const { return seth_be_to_cpu_16(id); }
void IPv4Packet::setId(uint16_t newId) { id = seth_cpu_to_be_16(newId); }

uint16_t IPv4Packet::getFragOff() const { return seth_be_to_cpu_16(frag_off); }
void IPv4Packet::setFragOff(uint16_t newFragOff) { frag_off = seth_cpu_to_be_16(newFragOff); }

uint8_t IPv4Packet::getTtl() const { return ttl; }
void IPv4Packet::setTtl(uint8_t newTTL) { ttl = newTTL; }

uint8_t IPv4Packet::getProtocol() const { return protocol; }
void IPv4Packet::setProtocol(uint8_t newProtocol) { protocol = newProtocol; }

uint16_t IPv4Packet::getChecksum() const { return _getIPv4Checksum(); }

std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> IPv4Packet::getDstAddr() const { return dst_addr; }
void IPv4Packet::setDstAddr(std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> newDstAddr) { dst_addr = newDstAddr; }

std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> IPv4Packet::getSrcAddr() const { return src_addr; }
void IPv4Packet::setSrcAddr(std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> newSrcAddr) { src_addr = newSrcAddr; }

uint32_t IPv4Packet::getPseudoChecksumLayer3(uint16_t length) const {

	// The IPv4 pseudo-header is defined in RFC 793, Section 3.1.
	struct ipv4_pseudo_header_t {
			uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN];
			uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN];
			uint8_t zero;
			uint8_t protocol;
			seth_be16_t length;
	} __seth_packed;

	ipv4_pseudo_header_t header;

	std::copy(src_addr.begin(), src_addr.end(), header.src_addr);
	std::copy(dst_addr.begin(), dst_addr.end(), header.dst_addr);

	header.zero = 0;
	header.protocol = protocol;
	header.length = seth_cpu_to_be_16(length);

	return compute_checksum_partial((uint8_t *)&header, sizeof(ipv4_pseudo_header_t), 0);
}


uint16_t IPv4Packet::getHeaderSize() const { return ihl * 4; }
uint16_t IPv4Packet::getHeaderSizeTotal() const { return IPv4Packet::getHeaderSize(); }
uint16_t IPv4Packet::getLayer3Size() const { return getHeaderSizeTotal() + getPayloadSize(); }

std::string IPv4Packet::asText() const {
	std::ostringstream oss;

	oss << IPPacket::asText() << std::endl;

	oss << "==> IPv4" << std::endl;

	oss << std::format("*Header Offset : {}", IPv4Packet::getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size   : {}", IPv4Packet::getHeaderSize()) << std::endl;

	oss << std::format("IP Header Len  : {} * 4 bytes = {}", getIHL(), getIHL() * 4) << std::endl;
	oss << std::format("DSCP           : {}", getDSCP()) << std::endl;
	oss << std::format("ECN            : {}", getECN()) << std::endl;
	oss << std::format("Total Length   : {}", getLayer3Size()) << std::endl;
	oss << std::format("ID             : {}", getId()) << std::endl;
	oss << std::format("Frag Offset    : {}", getFragOff()) << std::endl;
	oss << std::format("TTL            : {}", getTtl()) << std::endl;
	oss << std::format("Protocol       : {}", getProtocol()) << std::endl;
	oss << std::format("Checksum       : {:04X}", getChecksum()) << std::endl;

	std::array<uint8_t, SETH_PACKET_IPV4_IP_LEN> ip;

	ip = getDstAddr();
	oss << std::format("Destination IP : {}.{}.{}.{}", ip[0], ip[1], ip[2], ip[3]) << std::endl;

	ip = getSrcAddr();
	oss << std::format("Source IP      : {}.{}.{}.{}", ip[0], ip[1], ip[2], ip[3]) << std::endl;

	return oss.str();
}

std::string IPv4Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);
	oss << IPPacket::asBinary();

	ipv4_header_t header;

	header.ihl = ihl;
	header.version = 4;
	header.dscp = dscp;
	header.ecn = ecn;
	header.total_length = seth_cpu_to_be_16(getLayer3Size());
	header.id = seth_cpu_to_be_16(id);
	header.frag_off = seth_cpu_to_be_16(frag_off);
	header.ttl = ttl;
	header.protocol = protocol;

	std::copy(src_addr.begin(), src_addr.end(), header.src_addr);
	std::copy(dst_addr.begin(), dst_addr.end(), header.dst_addr);

	header.checksum = seth_cpu_to_be_16(_getIPv4Checksum());

	oss.write(reinterpret_cast<const char *>(&header), sizeof(ipv4_header_t));

	return oss.str();
}
