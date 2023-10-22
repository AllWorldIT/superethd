/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "SequenceDataGenerator.hpp"

namespace accl {

SequenceDataGenerator::SequenceDataGenerator(int len) { generate(len); }

void SequenceDataGenerator::generate(int len) {
	std::stringstream ss;
	char letter = 'A';
	uint8_t number = 0;
	int curlen = 0;

	while (curlen < len) {
		ss << letter;
		curlen++;
		for (number = '0'; number <= '9' && curlen < len; number++) {
			ss << number;
			curlen++;
		}
		// Update for the next letter
		if (letter == 'Z') {
			letter = 'A';
		} else {
			letter++;
		}
	}

	data = ss.str();
}

std::string SequenceDataGenerator::asString() const { return data; }

std::vector<uint8_t> SequenceDataGenerator::asBytes() const {
	std::vector<uint8_t> byteSequence(data.begin(), data.end());
	return byteSequence;
}

} // namespace accl