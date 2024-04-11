/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <fcntl.h>
#include <libnetlink.h>
#include <linux/if_ether.h>
#include <linux/if_tun.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

class TAPInterface {
	public:
		TAPInterface(const std::string &ifname);
		~TAPInterface();

		inline const int getFD() const;

		void setMTU(int mtu);
		inline uint16_t getMTU();

		inline bool isOnline();

		void start();

	private:
		int fd;
		unsigned char hwaddr[ETHER_ADDR_LEN];
		std::string ifname;
		uint16_t mtu;
		bool online;
};

/**
 * @brief Return FD for the TAP interface
 *
 * @return const int File descriptor
 */
inline const int TAPInterface::getFD() const { return this->fd; };

/**
 * @brief Return MTU for the TAP interface
 *
 * @return const uint16_t MTU
 */
inline uint16_t TAPInterface::getMTU() { return this->mtu; };

/**
 * @brief Check if the TAP interface is online
 *
 * @return true
 * @return false
 */
inline bool TAPInterface::isOnline() { return this->online; };
