/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "fdb_entry.hpp"
#include "libsethnetkit/ethernet_packet.hpp"
#include "remote_node.hpp"
#include <arpa/inet.h>
#include <cstdint>
#include <map>
#include <netinet/in.h>

/**
 * @brief FDB class
 *
 */
class FDB {
	public:
		FDB();
		//~FDB();

		std::shared_ptr<FDBEntry> add(const FDBMACAddress *mac, std::shared_ptr<RemoteNode> dest);
		std::shared_ptr<FDBEntry> get(const FDBMACAddress *mac) const;

		void expireEntries(int expire_time);

		const std::shared_ptr<FDBEntry> getRemoteNode(const FDBMACAddress *dst_mac) const;

		void dumpDebug();

	private:
		// FDB entry map based on MAC address
		std::map<std::array<uint8_t, SETH_PACKET_ETHERNET_MAC_LEN>, std::shared_ptr<FDBEntry>> fdb;
};
