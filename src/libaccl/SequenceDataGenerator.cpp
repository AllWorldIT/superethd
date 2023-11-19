/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include "SequenceDataGenerator.hpp"

namespace accl {

/**
 * @brief Construct a new Sequence Data Generator:: Sequence Data Generator object
 *
 * @param len Length of sequence data.
 */
SequenceDataGenerator::SequenceDataGenerator(int len) { generate(len); }

/**
 * @brief Generate sequence data of a specified length.
 *
 * @param len Length of sequence data to generate.
 */
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

/**
 * @brief Return data as a string.
 *
 * @return std::string String representation of the data.
 */
std::string SequenceDataGenerator::asString() const { return data; }

/**
 * @brief Return data as bytes.
 *
 * @return std::vector<uint8_t> Bytes representation of the data.
 */
std::vector<uint8_t> SequenceDataGenerator::asBytes() const {
	std::vector<uint8_t> byteSequence(data.begin(), data.end());
	return byteSequence;
}

} // namespace accl