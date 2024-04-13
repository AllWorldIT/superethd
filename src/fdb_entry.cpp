/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "fdb_entry.hpp"
#include <cstring>

/**
 * @brief Construct a new FDBEntry::FDBEntry object
 *
 * @param mac
 * @param dest
 */
FDBEntry::FDBEntry(const FDBMACAddress *mac, const std::shared_ptr<RemoteNode> dest) {
	this->last_seen = std::chrono::steady_clock::now();
	this->dest = dest;

	// Map the MAC to the key data
	this->mac = (FDBMACAddress *)this->key.data();
	memcpy(this->key.data(), mac->bytes, SETH_PACKET_ETHERNET_MAC_LEN);
}
