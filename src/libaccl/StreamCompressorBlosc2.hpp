/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "StreamCompressor.hpp"

extern "C" {
#include <blosc2.h>
}

namespace accl {

class StreamCompressorBlosc2 : public StreamCompressor {

	private:
		blosc2_context_s *cctx;
		blosc2_context_s *dctx;

	public:
		StreamCompressorBlosc2();

		~StreamCompressorBlosc2() override;

		void resetCompressionStream() override;
		void resetDecompressionStream() override;

		int compress(const char *input, size_t input_size, char *output, size_t max_output_size) override;

		int decompress(const char *input, size_t input_size, char *output, size_t max_output_size) override;
};

}