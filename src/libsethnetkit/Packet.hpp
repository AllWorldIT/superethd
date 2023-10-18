/*
 * Packet handling.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include "../common.hpp"
#include "../debug.hpp"
#include "../endian.hpp"
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
		// std::vector<uint8_t> rawData;
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