/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "ip_packet.hpp"
#include "ethernet_packet.hpp"
#include <format>
#include <ostream>
#include <sstream>

void IPPacket::_clear() { version = 0; }

IPPacket::IPPacket() : EthernetPacket() { _clear(); }

IPPacket::IPPacket(const std::vector<uint8_t> &data) : EthernetPacket(data) {
	_clear();
	parse(data);
}

IPPacket::~IPPacket() {}

void IPPacket::clear() {
	EthernetPacket::clear();
	IPPacket::_clear();
}

void IPPacket::parse(const std::vector<uint8_t> &data) {}

uint8_t IPPacket::getVersion() const { return version; }

void IPPacket::setVersion(uint8_t newVersion) { version = newVersion; }

uint16_t IPPacket::getHeaderOffset() const { return EthernetPacket::getHeaderOffset() + EthernetPacket::getHeaderSize(); }

std::string IPPacket::asText() const {
	std::ostringstream oss;

	oss << EthernetPacket::asText() << std::endl;

	oss << "==> IP" << std::endl;

	oss << std::format("*Header Offset : {}", IPPacket::getHeaderOffset()) << std::endl;
	oss << std::format("*Header Size   : {}", IPPacket::getHeaderSize()) << std::endl;

	oss << std::format("Version        : {}", getVersion()) << std::endl;

	return oss.str();
}

std::string IPPacket::asBinary() const { return EthernetPacket::asBinary(); }
