/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>

class StreamCompressor {
	public:
		virtual ~StreamCompressor() = default;

		virtual int compress(const char *input, size_t input_size, char *output, size_t max_output_size) = 0;
		virtual int decompress(const char *input, size_t input_size, char *output, size_t max_output_size) = 0;
};
