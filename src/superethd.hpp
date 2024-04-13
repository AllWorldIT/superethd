/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "codec.hpp"
#include <memory>
#include <netinet/in.h>
#include <string>
#include <vector>

int start_seth(const std::string ifname, int mtu, int tx_size, PacketHeaderOptionFormatType packet_format,
			   std::shared_ptr<struct sockaddr_storage> src_addr, std::vector<std::shared_ptr<struct sockaddr_storage>> dst_addrs,
			   int port);
