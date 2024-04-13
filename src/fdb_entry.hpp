/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "libsethnetkit/ethernet_packet.hpp"
#include "remote_node.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <netinet/in.h>

struct FDBMACAddress {
		uint8_t bytes[SETH_PACKET_ETHERNET_MAC_LEN];

		// Return string
		std::string toString() const {
			char str[SETH_PACKET_ETHERNET_MAC_LEN * 3];
			snprintf(str, sizeof(str), "%02x:%02x:%02x:%02x:%02x:%02x", this->bytes[0], this->bytes[1], this->bytes[2],
					 this->bytes[3], this->bytes[4], this->bytes[5]);
			return std::string(str);
		}
} ACCL_PACKED_ATTRIBUTES;

/**
 * @brief FDB Entry class
 *
 */
class FDBEntry {
	public:
		FDBEntry(const FDBMACAddress *mac, const std::shared_ptr<RemoteNode> dest);

		void setDest(const std::shared_ptr<RemoteNode> dest);
		inline const std::shared_ptr<RemoteNode> getDestination() const;

		inline const FDBMACAddress *getMAC() const;
		inline const std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> &getKey() const;
		inline bool isLocal() const;

		inline const std::chrono::steady_clock::time_point &getLastSeen() const;
		inline void setLastSeen(std::chrono::steady_clock::time_point last_seen);
		inline void updateLastSeen();

	private:
		FDBMACAddress *mac; // Pointer to storage area of key
		std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> key;

		std::shared_ptr<RemoteNode> dest;
		std::chrono::steady_clock::time_point last_seen;
};

/**
 * @brief Get the destination of the FDBEntry
 *
 * @return const sockaddr_storage*
 */
inline const std::shared_ptr<RemoteNode> FDBEntry::getDestination() const { return this->dest; }

/**
 * @brief Get the MAC address of the FDBEntry
 *
 * @return const FDBMACAddress*
 */
inline const FDBMACAddress *FDBEntry::getMAC() const { return this->mac; }

/**
 * @brief Check if the destination is local
 *
 * @return true
 * @return false
 */
inline bool FDBEntry::isLocal() const { return this->dest == nullptr; }

/**
 * @brief Get the key of the FDBEntry
 *
 * @return const std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN>&
 */
inline const std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> &FDBEntry::getKey() const { return this->key; }

/**
 * @brief Get the last time the entry was seen
 *
 * @return const std::chrono::steady_clock::time_point&
 */
inline const std::chrono::steady_clock::time_point &FDBEntry::getLastSeen() const { return this->last_seen; }

/**
 * @brief Set the last time the entry was seen
 *
 * @param last_seen
 */
inline void FDBEntry::setLastSeen(std::chrono::steady_clock::time_point last_seen) { this->last_seen = last_seen; }

/**
 * @brief Update the last time the entry was seen
 *
 */
inline void FDBEntry::updateLastSeen() { this->last_seen = std::chrono::steady_clock::now(); }