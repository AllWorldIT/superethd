/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "stream_compressor.hpp"

namespace accl {

StreamCompressor::StreamCompressor() : compression_level(5) {}

StreamCompressor::~StreamCompressor() = default;

} // namespace accl