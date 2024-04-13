/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "fdb.hpp"
#include "fdb_entry.hpp"
#include "libaccl/logger.hpp"
#include "util.hpp"
#include <cstring>
#include <memory>

/**
 * @brief Construct a new FDB::FDB object
 *
 */
FDB::FDB() {}

/**
 * @brief Add a new entry to the FDB
 *
 * @param mac
 * @param dest
 * @return std::shared_ptr<FDBEntry>
 */
std::shared_ptr<FDBEntry> FDB::add(const FDBMACAddress *mac, std::shared_ptr<RemoteNode> dest) {
	// Build MAC key to add to FDB
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> key;
	memcpy(key.data(), mac->bytes, SETH_PACKET_ETHERNET_MAC_LEN);

	// Check if entry already exists
	auto it = fdb.find(key);
	if (it != fdb.end()) {
		// Entry already exists
		return it->second;
	}

	// Create entry
	std::shared_ptr<FDBEntry> entry = std::make_shared<FDBEntry>(mac, dest);
	// Add entry
	this->fdb[key] = entry;

	return entry;
}

/**
 * @brief Get an entry from the FDB
 *
 * @param mac
 * @return std::shared_ptr<FDBEntry>
 */

std::shared_ptr<FDBEntry> FDB::get(const FDBMACAddress *mac) const {
	// Build MAC key to get from FDB
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> key;
	memcpy(key.data(), mac->bytes, SETH_PACKET_ETHERNET_MAC_LEN);

	// Lookup entry
	auto it = fdb.find(key);
	if (it == fdb.end()) {
		// Return nullptr if not found
		return nullptr;
	}
	return it->second;
}

/**
 * @brief Expire entries in the FDB
 *
 */
void FDB::expireEntries(int expire_time) {
	auto now = std::chrono::steady_clock::now();

	// Iterate over all entries
	for (auto it = fdb.begin(); it != fdb.end();) {
		// Check if entry is expired
		auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - it->second->getLastSeen()).count();
		if (diff > expire_time) {
			LOG_DEBUG_INTERNAL("FDB: Expired entry: ", it->second->getMAC()->toString());
			// Remove entry
			it = fdb.erase(it);
		} else {
			// Move to next entry
			++it;
		}
	}
}

const std::shared_ptr<FDBEntry> FDB::getRemoteNode(const FDBMACAddress *mac) const {
	// Build MAC key to add to FDB
	std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN> key;
	memcpy(key.data(), mac->bytes, SETH_PACKET_ETHERNET_MAC_LEN);

	// Check to see if we can find the remote node by MAC address
	auto it = fdb.find(key);
	if (it != fdb.end()) {
		return it->second;
	}

	return nullptr;
}

/**
 * @brief Dump debug information about the FDB
 *
 */
void FDB::dumpDebug() {
	LOG_DEBUG_INTERNAL("@@@@@@@@@@@@@@@@@@@@ FDB: Dumping FDB @@@@@@@@@@@@@@@@@@@@");

	auto now = std::chrono::steady_clock::now();

	for (auto &entry : this->fdb) {
		const FDBMACAddress *mac = entry.second->getMAC();
		// Build string from mac address .bytes
		std::string mac_str = std::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", mac->bytes[0], mac->bytes[1], mac->bytes[2],
										  mac->bytes[3], mac->bytes[4], mac->bytes[5]);

		// Work out what the IP is
		std::string ip_str;
		if (entry.second->isLocal()) {
			ip_str = "LOCAL";
		} else {
			ip_str = get_ipstr(entry.second->getDestination()->getNodeAddr().get());
		}

		LOG_DEBUG_INTERNAL("    - ", mac_str, " => ", ip_str, " (last seen: ",
						   std::chrono::duration_cast<std::chrono::seconds>(now - entry.second->getLastSeen()).count(), "s)");
	}
}
