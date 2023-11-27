/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef __T_FRAMEWORK_H__
#define __T_FRAMEWORK_H__

#define CATCH_CONFIG_MAIN

#include "Codec.hpp"
#include "PacketBuffer.hpp"
#include "debug.hpp"
#include "libaccl/BufferPool.hpp"
#include "libaccl/Logger.hpp"
#include "libaccl/SequenceDataGenerator.hpp"
#include "libsethnetkit/EthernetPacket.hpp"
#include "libsethnetkit/ICMPv4Packet.hpp"
#include "libsethnetkit/ICMPv6Packet.hpp"
#include "libsethnetkit/IPPacket.hpp"
#include "libsethnetkit/IPv4Packet.hpp"
#include "libsethnetkit/IPv6Packet.hpp"
#include "libsethnetkit/Packet.hpp"
#include "libsethnetkit/TCPPacket.hpp"
#include "libsethnetkit/UDPPacket.hpp"
#include "libsethnetkit/checksum.hpp"
#include <catch2/catch_all.hpp>
#include <iostream>
#include <memory>

#include "util.hpp"

#endif