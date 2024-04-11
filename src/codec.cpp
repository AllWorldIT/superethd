/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "codec.hpp"

std::string PacketHeaderOptionFormatTypeToString(PacketHeaderOptionFormatType type) {
	switch (type) {
	case PacketHeaderOptionFormatType::NONE:
		return "none";
	case PacketHeaderOptionFormatType::COMPRESSED_LZ4:
		return "lz4";
	case PacketHeaderOptionFormatType::COMPRESSED_ZSTD:
		return "zstd";
	default:
		return "unknown";
	}
}
