/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "stream_compressor.hpp"

extern "C" {
#include <zstd.h>
}

namespace accl {

class StreamCompressorZSTD : public StreamCompressor {
		ZSTD_CCtx *cctx;
		ZSTD_DCtx *dctx;

	public:
		StreamCompressorZSTD();

		~StreamCompressorZSTD() override;

		void resetCompressionStream() override;
		void resetDecompressionStream() override;

		int compress(const char *input, size_t input_size, char *output, size_t max_output_size) override;

		int decompress(const char *input, size_t input_size, char *output, size_t max_output_size) override;

		const std::string strerror(int err) override;
};

} // namespace accl