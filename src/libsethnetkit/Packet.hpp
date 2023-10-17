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
		std::vector<uint8_t> rawData;

	private:
		void _clear();

	public:
		Packet();
		Packet(const std::vector<uint8_t> &data);

		virtual ~Packet();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		const uint8_t *getPointer() const;
		const std::vector<uint8_t> &getData() const;

		uint16_t getSize() const;
		void resize(uint16_t newSize);

		void printHex() const;
		void printText() const;

		int compare(void *cmp, uint16_t len);

		std::string asHex() const;
		std::string asText() const;
		std::string asBinary() const;
};