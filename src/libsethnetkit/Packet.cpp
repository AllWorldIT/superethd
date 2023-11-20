/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "Packet.hpp"

void Packet::_clear() { payload.clear(); }

Packet::Packet() { _clear(); };

Packet::Packet(const std::vector<uint8_t> &data) : Packet() {
	_clear();
	parse(data);
}

// Use default destructor
Packet::~Packet() = default;

void Packet::clear() { Packet::_clear(); }

void Packet::parse(const std::vector<uint8_t> &data) {}

// Payload handling
void Packet::addPayload(const std::vector<uint8_t> &data) { payload = data; }

const uint8_t *Packet::getPayloadPointer() const { return payload.data(); }
const std::vector<uint8_t> &Packet::getPayload() const { return payload; };

uint16_t Packet::getPayloadSize() const { return payload.size(); }
void Packet::resizePayload(uint16_t newSize) { payload.resize(newSize); }

uint16_t Packet::getHeaderOffset() const { return 0; }
uint16_t Packet::getHeaderSize() const { return 0; }
uint16_t Packet::getPacketSize() const { return getHeaderOffset() + getHeaderSize(); }

// Print packet handling
void Packet::printHex() const {
	std::cout << "==> Hex Dump" << std::endl;
	std::cout << asHex() << std::endl;
}

void Packet::printText() const { std::cout << asText() << std::endl; }

bool Packet::compare(std::string bin) {
	std::string pkt = asBinary();
	return pkt == bin;
}

std::string Packet::asHex() const {
	std::ostringstream oss;
	uint16_t count = 0; // For keeping track of byte count

	for (const auto &byte : asBinary()) {
		// If it's the start of a new line, print the position
		if (count % 16 == 0) {
			if (count != 0) {
				oss << std::endl; // Add a new line for every line except the first
			}

			// oss << std::hex << std::setfill('0') << std::setw(4) << count << ": ";
			oss << std::format("{:04X}", count) << ": ";
		} else {
			oss << " ";
		}

		// Print the byte in hex format
		oss << std::format("{:02X}", static_cast<uint8_t>(byte));

		count++;
	}

	return oss.str();
}

std::string Packet::asText() const {
	std::ostringstream oss;

	oss << "==> Packet" << std::endl;
	oss << "Size           : " << asBinary().size() << std::endl;

	return oss.str();
}

std::string Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);
	return oss.str();
}