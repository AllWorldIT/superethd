#pragma once

#include "../common.hpp"
#include "../endian.hpp"
#include "../debug.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// Base Packet Class
class Packet {
	protected:
		//std::vector<uint8_t> rawData;
		std::vector<uint8_t> payload;

	private:
		void _clear();

	public:
		Packet();
		Packet(const std::vector<uint8_t> &data);

		virtual ~Packet();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		void addPayload(const std::vector<uint8_t> &data);
		const uint8_t *getPayloadPointer() const;
		const std::vector<uint8_t> &getPayload() const;
		uint16_t getPayloadSize() const;
		void resizePayload(uint16_t newSize);

		void printHex() const;
		void printText() const;

		int compare(void *cmp, uint16_t len);

		std::string asHex() const;
		std::string asText() const;
		std::string asBinary() const;
};