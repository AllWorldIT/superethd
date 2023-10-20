/*
 * Main SETH program.
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

#include <string>
extern "C" {
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include "common.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "superethd.hpp"
#include "util.hpp"

static struct option long_options[] = {
	{"version", no_argument, 0, 'v'},	{"help", no_argument, 0, 'h'},		  {"ifname", required_argument, 0, 'i'},
	{"src", required_argument, 0, 's'}, {"dst", required_argument, 0, 'd'},	  {"port", required_argument, 0, 'p'},
	{"mtu", required_argument, 0, 'm'}, {"tsize", required_argument, 0, 't'}, {0, 0, 0, 0}};

void print_help() {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "    -v, --version                 Print version\n");
	fprintf(stderr, "    -h, --help                    Print this help\n");
	fprintf(stderr,
			"    -m, --mtu=<MTU>               Specify the interface MTU of between %i and\n"
			"                                  %i (default is 1500)\n",
			SETH_MIN_MTU_SIZE, SETH_MAX_MTU_SIZE);
	fprintf(stderr,
			"    -t, --txsize=<TSIZE>          Specify the maximum transmissions packet size\n"
			"                                  of between %i and %i (default is 1500)\n",
			SETH_MIN_TXSIZE, SETH_MAX_TXSIZE);
	fprintf(stderr,
			"    -s, --src=<SOURCE>            Specify the source IPv4/IPv6 address\n"
			"                                  (mandatory)\n");
	fprintf(stderr,
			"    -d, --dst=<DESTINATION>       Specify the destination IPv4/IPv6 address\n"
			"                                  (mandatory)\n");
	fprintf(stderr,
			"    -p, --port=<PORT>             Specify the UDP port, between 1 and 65535\n"
			"                                  (default is 58023)\n");
	fprintf(stderr,
			"    -i, --ifname=<IFNAME>         Specify interface name to use up to %d\n"
			"                                  characters (default is \"%s\")\n",
			IFNAMSIZ, SETH_DEFAULT_TUNNEL_NAME);
}

int main(int argc, char *argv[]) {
	int c;
	int option_index = 0;
	int mtu_value = 1500;
	int tsize_value = 1500;
	int port_value = 58023;
	char *src_value = NULL;
	char *dst_value = NULL;
	std::string ifname_value = SETH_DEFAULT_TUNNEL_NAME;

	fprintf(stderr, "Super Ethernet Tunnel v%s - Copyright (c) 2023, AllWorldIT.\n", VERSION);
	fprintf(stderr, "\n");

	while (1) {
		c = getopt_long(argc, argv, "vhm:s:r:d:p:i:", long_options, &option_index);

		// Detect end of the options
		if (c == -1) {
			break;
		}

		switch (c) {
			case 'v':
				printf("Version: %s\n", VERSION);
				return 0;
			case 'h':
				fprintf(stderr, "\n");
				print_help();
				return 0;
			case 'm':
				mtu_value = atoi(optarg);
				if (mtu_value < SETH_MIN_MTU_SIZE || mtu_value > SETH_MAX_MTU_SIZE) {
					FPRINTF("ERROR: Invalid MTU value. It should be between %i and %i.", SETH_MIN_MTU_SIZE, SETH_MAX_MTU_SIZE);
					return 1;
				}
				break;
			case 't':
				tsize_value = atoi(optarg);
				if (tsize_value < SETH_MIN_TXSIZE || tsize_value > SETH_MAX_TXSIZE) {
					FPRINTF("ERROR: Invalid TX_SIZE value. It should be between %i and %i.", SETH_MIN_TXSIZE, SETH_MAX_TXSIZE);
					return 1;
				}
				break;
			case 's':
				src_value = optarg;
				break;
			case 'd':
				dst_value = optarg;
				break;
			case 'p':
				port_value = atoi(optarg);
				if (port_value < 1 || port_value > 65535) {
					FPRINTF("Invalid port value. It should be between 1 and 65535.");
					return 1;
				}
				break;
			case 'i':  // Handling the ifname option
				if (strlen(optarg) >= IFNAMSIZ) {
					FPRINTF("ERROR: Invalid interface name. It should be less than %d characters.", IFNAMSIZ);
					return 1;
				}
				ifname_value.assign(optarg);
				break;
			case '?':
				return 1;
			default:
				abort();
		}
	}

	// Check if mandatory options src and dst are provided
	if (!src_value || !dst_value) {
		FPRINTF("ERROR: Both --src (-s) and --dst (-d) options are mandatory.");
		print_help();
		return 1;
	}

	// Convert local address to a IPv6 sockaddr
	struct in6_addr src_addr;
	char src_addr_str[INET6_ADDRSTRLEN];
	if (to_sin6addr(src_value, &src_addr)) {
		if (inet_ntop(AF_INET6, &src_addr, src_addr_str, sizeof(src_addr_str)) == NULL) {
			FPRINTF("ERROR: Failed to convert source address '%s' to IPv6 address: %s", src_value, strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		FPRINTF("ERROR: Failed to convert source address '%s' to IPv6 address", src_value);
		exit(EXIT_FAILURE);
	}

	// Convert remote address to a IPv6 sockaddr
	struct in6_addr dst_addr;
	char dst_addr_str[INET6_ADDRSTRLEN];
	if (to_sin6addr(dst_value, &dst_addr)) {
		if (inet_ntop(AF_INET6, &dst_addr, dst_addr_str, sizeof(dst_addr_str)) == NULL) {
			FPRINTF("ERROR: Failed to convert destination address '%s' to IPv6 address: %s", dst_value, strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		FPRINTF("ERROR: Failed to convert source address '%s' to IPv6 address", dst_value);
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "Interface...: %s\n", ifname_value.c_str());
	fprintf(stderr, "Source......: %s\n", src_addr_str);
	fprintf(stderr, "Destination.: %s\n", dst_addr_str);
	fprintf(stderr, "UDP Port....: %d\n", port_value);
	fprintf(stderr, "MTU.........: %d\n", mtu_value);
	fprintf(stderr, "MSS.........: %d\n", tsize_value);
	fprintf(stderr, "\n");

	start_set(ifname_value, &src_addr, &dst_addr, port_value, mtu_value, tsize_value);

	return 0;
}
