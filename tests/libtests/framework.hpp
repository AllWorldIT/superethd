/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef __T_FRAMEWORK_H__
#define __T_FRAMEWORK_H__

#define CATCH_CONFIG_MAIN

#include "codec.hpp"
#include "packet_buffer.hpp"
#include "debug.hpp"
#include "libaccl/buffer_pool.hpp"
#include "libaccl/logger.hpp"
#include "libaccl/sequence_data_generator.hpp"
#include "libsethnetkit/ethernet_packet.hpp"
#include "libsethnetkit/icmpv4_packet.hpp"
#include "libsethnetkit/icmpv6_packet.hpp"
#include "libsethnetkit/ip_packet.hpp"
#include "libsethnetkit/ipv4_packet.hpp"
#include "libsethnetkit/ipv6_packet.hpp"
#include "libsethnetkit/packet.hpp"
#include "libsethnetkit/tcp_packet.hpp"
#include "libsethnetkit/udp_packet.hpp"
#include "libsethnetkit/checksum.hpp"
#include <catch2/catch_all.hpp>
#include <iostream>
#include <memory>
#include <random>

#include "util.hpp"

#endif