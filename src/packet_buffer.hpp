/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "libaccl/buffer.hpp"
#include <cstdint>
#include <sys/socket.h>
#include <sys/types.h>

/**
 * @brief PacketBuffer class
 *
 */
class PacketBuffer : public accl::Buffer {
	public:
		inline PacketBuffer(std::size_t size);

		inline uint32_t getPacketSequenceKey() const;
		inline void setPacketSequenceKey(uint32_t k);

		inline void *getPacketSourceData();
		inline const sockaddr_storage &getPacketSource() const;
		inline void setPacketSource(const sockaddr_storage &source);

		inline uint64_t getPacketSourceKey() const;
		inline void setPacketSourceKey(uint64_t key);

		inline bool operator<(const PacketBuffer &other) const;

	private:
		uint32_t packet_sequence_key;

		sockaddr_storage source;
		uint64_t source_key;
};

/**
 * @brief Construct a new PacketBuffer::PacketBuffer object
 *
 * @param size Packet buffer size.
 */
inline PacketBuffer::PacketBuffer(std::size_t size) : Buffer(size), packet_sequence_key(0), source_key(0){};

/**
 * @brief Get the packet sequence key
 *
 * @return uint32_t Packet sequence key
 */
inline uint32_t PacketBuffer::getPacketSequenceKey() const { return this->packet_sequence_key; }

/**
 * @brief Set the packet sequence key
 *
 * @param k Packet sequence key
 */
inline void PacketBuffer::setPacketSequenceKey(uint32_t k) { this->packet_sequence_key = k; };

/**
 * @brief Get the packet source data
 *
 * @return void* Pointer to source data
 */
inline void *PacketBuffer::getPacketSourceData() { return &this->source; }

/**
 * @brief Get the packet source
 *
 * @return const sockaddr_storage& Source data
 */
inline const sockaddr_storage &PacketBuffer::getPacketSource() const { return this->source; }

/**
 * @brief Set the packet source
 *
 * @param source Source data
 */
inline void PacketBuffer::setPacketSource(const sockaddr_storage &source) { this->source = source; }

/**
 * @brief Get the packet source key
 *
 * @return uint64_t Source key
 */
inline uint64_t PacketBuffer::getPacketSourceKey() const { return this->source_key; };

/**
 * @brief
 *
 * @param key Source key
 */
inline void PacketBuffer::setPacketSourceKey(uint64_t key) { this->source_key = key; };

/**
 * @brief Compare two PacketBuffer objects
 *
 * @param other Other PacketBuffer object
 * @return true If this object is less than the other object
 * @return false If this object is not less than the other object
 */
inline bool PacketBuffer::operator<(const PacketBuffer &other) const {
	return this->packet_sequence_key < other.packet_sequence_key;
}
