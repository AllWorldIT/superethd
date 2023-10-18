/*
 * Sequence data generator.
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

#include "SequenceDataGenerator.hpp"

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
