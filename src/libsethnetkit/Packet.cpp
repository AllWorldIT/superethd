#include "Packet.hpp"

Packet::Packet() = default;

Packet::Packet(const std::vector<uint8_t> &data) {
	// Base class parseing of the data we got
	parse(data);
}

// Use default destructor
Packet::~Packet() = default;

void Packet::parse(const std::vector<uint8_t> &data) { rawData = data; }

const uint8_t *Packet::getPointer() const { return rawData.data(); }
const std::vector<uint8_t> &Packet::getData() const { return rawData; };

uint16_t Packet::getSize() const { return rawData.size(); }
void Packet::resize(uint16_t newSize) { rawData.resize(newSize); }

void Packet::printHex() const { std::cout << asHex() << std::endl; }
void Packet::printText() const { std::cout << asText() << std::endl; }

int Packet::compare(void *cmp, uint16_t len) { return std::memcmp(getPointer(), cmp, len); }

std::string Packet::asHex() const {
	std::ostringstream oss;
	uint16_t count = 0; // For keeping track of byte count

	for (const auto &byte : rawData) {
		// If it's the start of a new line, print the position
		if (count % 16 == 0) {
			if (count != 0) {
				oss << std::endl; // Add a new line for every line except the first
			}
			oss << std::hex << std::setfill('0') << std::setw(4) << count << ": ";
		}

		// Print the byte in hex format
		oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";

		count++;
	}

	return oss.str();
}

std::string Packet::asText() const {
	std::ostringstream oss;

	oss << "==> Packet" << std::endl;
	oss << "Size           : " << getSize() << std::endl;

	return oss.str();
}

std::string Packet::asBinary() const {
	std::ostringstream oss(std::ios::binary);
	return oss.str();
}