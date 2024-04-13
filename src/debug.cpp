/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include "debug.hpp"
#include <cstdint>
#include <format>
#include <ostream>
#include <sstream>

/**
 * @brief Dump a string as hex.
 *
 * @param buffer String to dump.
 * @return std::string Hex representation of the string.
 */
std::string hex_dump(const std::string buffer) {

	std::ostringstream oss;
	uint16_t count = 0; // For keeping track of byte count

	for (const auto &byte : buffer) {
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