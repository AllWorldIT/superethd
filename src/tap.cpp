/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "tap.hpp"
#include "threads.hpp"
#include <fcntl.h>
#include <iostream>
#include <libnetlink.h>
#include <linux/if_tun.h>
#include <string>
#include <sys/ioctl.h>

/**
 * @brief Create a tap interface.
 *
 * @param ifname Interface name.
 * @param tdata Thread data.
 */
void create_tap_interface(const std::string ifname, struct ThreadData *tdata) {
	struct ifreq ifr;
	int fd, err;

	if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
		std::cerr << std::format("ERROR: Cannot open TUN/TAP device file: {}", strerror(errno)) << std::endl;
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

	// Set interface name
	strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ);
	(void)memcpy(tdata->tap_device.ifname, ifr.ifr_name, sizeof ifr.ifr_name);

	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
		std::cerr << std::format("ERROR: Cannot ioctl TUNSETIFF: {} (return: {})", strerror(errno), err) << std::endl;
		close(fd);
		exit(1);
	}

	// Grab hardware address
	struct ifreq ifr_hw;
	(void)memcpy(ifr_hw.ifr_name, tdata->tap_device.ifname, sizeof(tdata->tap_device.ifname));
	if (ioctl(fd, SIOCGIFHWADDR, &ifr_hw) == -1) {
		std::cerr << std::format("ERROR: Can't get link-layer address: {}", strerror(errno)) << std::endl;
		close(fd);
		exit(1);
	}
	(void)memcpy(tdata->tap_device.hwaddr, ifr_hw.ifr_hwaddr.sa_data, ETH_ALEN);

	tdata->tap_device.fd = fd;
}

/**
 * @brief Destroy the TAP interface.
 *
 * @param tdata Thread data.
 */
void destroy_tap_interface(struct ThreadData *tdata) {
	if (tdata->tap_device.fd)
		close(tdata->tap_device.fd);
}

/**
 * @brief Initialize the TAP interface.
 *
 * @param tdata Thread data.
 * @return int 0 on success.
 */
int start_tap_interface(struct ThreadData *tdata) {
	int sockfd;
	struct ifreq ifr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		std::cerr << std::format("ERROR: Error opening socket to set interface UP: {}", strerror(errno)) << std::endl;
		exit(1);
	}

	(void)memcpy(ifr.ifr_name, tdata->tap_device.ifname, IFNAMSIZ);

	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) == -1) {
		std::cerr << std::format("ERROR: Error getting interface flags: {}", strerror(errno)) << std::endl;
		close(sockfd);
		exit(1);
	}

	ifr.ifr_flags |= IFF_UP;

	if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) == -1) {
		std::cerr << std::format("ERROR: Error setting interface up: {}", strerror(errno)) << std::endl;
		close(sockfd);
		exit(1);
	}

	close(sockfd);
	return 0;
}

int set_interface_mtu(struct ThreadData *tdata) {
	int sockfd;
	struct ifreq ifr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		std::cerr << std::format("ERROR: Error opening socket to set interface MTU: {}", strerror(errno)) << std::endl;
		exit(1);
	}

	(void)memcpy(ifr.ifr_name, tdata->tap_device.ifname, IFNAMSIZ);

	// Set the MTU
	ifr.ifr_mtu = tdata->mtu;

	if (ioctl(sockfd, SIOCSIFMTU, &ifr) == -1) {
		std::cerr << std::format("ERROR: Error setting interface MTU: {}", strerror(errno)) << std::endl;
		close(sockfd);
		exit(1);
	}

	close(sockfd);
	return 0;
}
