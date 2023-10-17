#ifndef __T_FRAMEWORK_H__
#define __T_FRAMEWORK_H__

#define CATCH_CONFIG_MAIN

#include <catch2/catch_all.hpp>

#include "libsethnetkit/EthernetPacket.hpp"
#include "libsethnetkit/ICMPv4Packet.hpp"
#include "libsethnetkit/IPPacket.hpp"
#include "libsethnetkit/IPv4Packet.hpp"
#include "libsethnetkit/IPv6Packet.hpp"
#include "libsethnetkit/Packet.hpp"
#include "libsethnetkit/TCPPacket.hpp"
#include "libsethnetkit/UDPPacket.hpp"
#include "libsethnetkit/checksum.hpp"

#include "util.hpp"

extern "C" {
#include <stdlib.h>
}

#endif