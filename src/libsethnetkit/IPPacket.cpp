/*
 * IP packet handling.
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

#include "IPPacket.hpp"
#include "EthernetPacket.hpp"

void IPPacket::_clear() { version = 0; }

IPPacket::IPPacket() : EthernetPacket() { _clear(); }

IPPacket::IPPacket(const std::vector<uint8_t> &data) : EthernetPacket(data) {}

IPPacket::~IPPacket() {}

void IPPacket::clear() {
	EthernetPacket::clear();
	IPPacket::_clear();
}

void IPPacket::parse(const std::vector<uint8_t> &data) {
	// bye bye world
	EthernetPacket::parse(data);
}

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

std::string IPPacket::asBinary() const {
	return EthernetPacket::asBinary();
}
