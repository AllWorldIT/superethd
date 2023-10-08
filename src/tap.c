#include "tap.h"

#include <errno.h>
#include <fcntl.h>
#include <libnetlink.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "debug.h"
#include "threads.h"

void create_tap_interface(char *ifname, struct ThreadData *tdata) {
	struct ifreq ifr;
	int fd, err;

	if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
		FPRINTF("Cannot open TUN/TAP device file: %s", strerror(errno));
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

	// Set interface name
	if (*ifname) {
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		(void)memcpy(tdata->tap_device.ifname, ifr.ifr_name, sizeof ifr.ifr_name);
	}
	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
		FPRINTF("Cannot ioctl TUNSETIFF: %s", strerror(errno));
		close(fd);
		exit(1);
	}

	// Grab hardware address
	struct ifreq ifr_hw;
	(void)memcpy(ifr_hw.ifr_name, tdata->tap_device.ifname, sizeof(tdata->tap_device.ifname));
	if (ioctl(fd, SIOCGIFHWADDR, &ifr_hw) == -1) {
		FPRINTF("Can't get link-layer address: %s", strerror(errno));
		close(fd);
		exit(1);
	}
	(void)memcpy(tdata->tap_device.hwaddr, ifr_hw.ifr_hwaddr.sa_data, ETH_ALEN);

	tdata->tap_device.fd = fd;
}

void destroy_tap_interface(struct ThreadData *tdata) {
	if (tdata->tap_device.fd) close(tdata->tap_device.fd);
}

int start_tap_interface(struct ThreadData *tdata) {
	int sockfd;
	struct ifreq ifr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		FPRINTF("Error opening socket to set interface UP: %s", strerror(errno));
		exit(1);
	}

	(void)memcpy(ifr.ifr_name, tdata->tap_device.ifname, IFNAMSIZ);

	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) == -1) {
		FPRINTF("Error getting interface flags: %s", strerror(errno));
		close(sockfd);
		exit(1);
	}

	ifr.ifr_flags |= IFF_UP;

	if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) == -1) {
		FPRINTF("Error setting interface up: %s", strerror(errno));
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
		FPRINTF("Error opening socket to set interface MTU: %s", strerror(errno));
		exit(1);
	}

	(void)memcpy(ifr.ifr_name, tdata->tap_device.ifname, IFNAMSIZ);

	// Set the MTU
	ifr.ifr_mtu = tdata->mtu;

	if (ioctl(sockfd, SIOCSIFMTU, &ifr) == -1) {
		FPRINTF("Error setting interface MTU: %s", strerror(errno));
		close(sockfd);
		exit(1);
	}

	close(sockfd);
	return 0;
}
