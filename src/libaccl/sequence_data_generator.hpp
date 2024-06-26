/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace accl {

class SequenceDataGenerator {
	private:
		std::string data;

	public:
		SequenceDataGenerator(int len);

		void generate(int len);

		std::string asString() const;
		std::vector<uint8_t> asBytes() const;
};

} // namespace accl