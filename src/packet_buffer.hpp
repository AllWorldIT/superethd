/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "libaccl/buffer.hpp"
#include <cstdint>

class PacketBuffer : public accl::Buffer {
	private:
		uint32_t key;

	public:
		inline PacketBuffer(std::size_t size);

		inline uint32_t getKey() const;

		inline void setKey(uint32_t k);

		inline bool operator<(const PacketBuffer &other) const;
};

inline PacketBuffer::PacketBuffer(std::size_t size) : Buffer(size), key(0){};

inline uint32_t PacketBuffer::getKey() const { return key; }

inline void PacketBuffer::setKey(uint32_t k) { key = k; };

inline bool PacketBuffer::operator<(const PacketBuffer &other) const { return key < other.key; }