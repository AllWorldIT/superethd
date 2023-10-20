/*
 * Testing framework.
 * Copyright (C) 2023, AllWorldIT.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __T_FRAMEWORK_H__
#define __T_FRAMEWORK_H__

#define CATCH_CONFIG_MAIN

#include <catch2/catch_all.hpp>

#include "libsethnetkit/EthernetPacket.hpp"
#include "libsethnetkit/ICMPv4Packet.hpp"
#include "libsethnetkit/ICMPv6Packet.hpp"
#include "libsethnetkit/IPPacket.hpp"
#include "libsethnetkit/IPv4Packet.hpp"
#include "libsethnetkit/IPv6Packet.hpp"
#include "libsethnetkit/Packet.hpp"
#include "libsethnetkit/TCPPacket.hpp"
#include "libsethnetkit/SequenceDataGenerator.hpp"
#include "libsethnetkit/UDPPacket.hpp"
#include "libsethnetkit/checksum.hpp"

#include "util.hpp"

extern "C" {
#include <stdlib.h>
}

#endif