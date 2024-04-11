/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "tap_interface.hpp"
#include "exceptions.hpp"
#include "libaccl/logger.hpp"
#include "threads.hpp"
#include <fcntl.h>
#include <format>
#include <libnetlink.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * @brief Construct a new TAPInterface::TAPInterface object
 *
 * @param ifname Interface name
 */
TAPInterface::TAPInterface(const std::string &ifname) {
	// Open TUN/TAP device file to create an interface
	if ((this->fd = open("/dev/net/tun", O_RDWR)) < 0) {
		throw SuperEthernetTunnelRuntimeException("Cannot open TUN/TAP device file: " + std::string(strerror(errno)));
	}

	// Save interface name
	this->ifname = ifname;
	this->mtu = 1500;

	// Create TAP interface
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	memcpy(ifr.ifr_name, this->ifname.data(), this->ifname.length());
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if (ioctl(this->fd, TUNSETIFF, (void *)&ifr) == -1) {
		int res = errno;
		close(this->fd);
		throw SuperEthernetTunnelRuntimeException("Cannot ioctl TUNSETIFF on '" + ifname + "': " + std::string(strerror(res)));
	}

	// Grab hardware address
	struct ifreq ifr_hw;
	memset(&ifr_hw, 0, sizeof(ifr_hw));
	memcpy(ifr_hw.ifr_name, this->ifname.data(), this->ifname.length());
	if (ioctl(this->fd, SIOCGIFHWADDR, &ifr_hw) == -1) {
		int res = errno;
		close(this->fd);
		throw SuperEthernetTunnelRuntimeException("Cannot get link-layer address: " + std::string(strerror(res)));
	}

	// Save hardware address
	memcpy(hwaddr, ifr_hw.ifr_hwaddr.sa_data, ETH_ALEN);

	// Log the MAC address
	LOG_INFO(
		"Created TAP interface '", this->ifname, "' with MAC address '",
		std::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]),
		"'");
}

/**
 * @brief Destroy the TAP interface.
 */
TAPInterface::~TAPInterface() {
	if (this->fd) {
		close(this->fd);
	}
}

/**
 * @brief Set the MTU of the interface.
 *
 * @param mtu MTU to set.
 */
void TAPInterface::setMTU(int mtu) {
	// Open a socket to set the MTU
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		throw SuperEthernetTunnelRuntimeException("Cannot open socket to set interface '" + this->ifname +
												  "' MTU: " + std::string(strerror(errno)));
	}

	// Build structure to set MTU
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	// Set interface name of the device to change the MTU for
	memcpy(ifr.ifr_name, this->ifname.data(), this->ifname.length());
	// Set the MTU
	ifr.ifr_mtu = mtu;

	// Set the MTU
	if (ioctl(sockfd, SIOCSIFMTU, &ifr) == -1) {
		int res = errno;
		close(sockfd);
		throw SuperEthernetTunnelRuntimeException("Cannot set interface MTU on '" + this->ifname +
												  "': " + std::string(strerror(res)));
	}

	close(sockfd);

	// Save the MTU
	this->mtu = mtu;

	// Interface is currently down
	this->online = false;
}

/**
 * @brief Start the interface.
 */
void TAPInterface::start() {
	// Open a socket to bring the interface up
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		throw SuperEthernetTunnelRuntimeException("Cannot open socket to set interface UP: " + std::string(strerror(errno)));
	}

	// Copy interface name into the ifreq structure to match the device to bring up
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	memcpy(ifr.ifr_name, this->ifname.data(), this->ifname.length());
	// Create the device
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) == -1) {
		int res = errno;
		close(sockfd);
		throw SuperEthernetTunnelRuntimeException("Cannot get interface flags for '" + this->ifname +
												  "': " + std::string(strerror(res)));
	}

	// Bring interface up
	ifr.ifr_flags |= IFF_UP;
	if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) == -1) {
		int res = errno;
		close(sockfd);
		throw SuperEthernetTunnelRuntimeException("Cannot set interface UP: " + std::string(strerror(res)));
	}

	close(sockfd);

	// Interface is now online
	this->online = true;
}
