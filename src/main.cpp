/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <string>

#include <arpa/inet.h>
#include <getopt.h>
#include <net/if.h>
#include <string.h>

#include "common.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "libaccl/Logger.hpp"
#include "superethd.hpp"
#include "util.hpp"

static struct option long_options[] = {
	{"version", no_argument, 0, 'v'},	{"help", no_argument, 0, 'h'},		  {"ifname", required_argument, 0, 'i'},
	{"src", required_argument, 0, 's'}, {"dst", required_argument, 0, 'd'},	  {"port", required_argument, 0, 'p'},
	{"mtu", required_argument, 0, 'm'}, {"tsize", required_argument, 0, 't'}, {0, 0, 0, 0}};

void print_help() {
	CERR("Usage:");
	CERR("    -v, --version                 Print version");
	CERR("    -h, --help                    Print this help");
	CERR("    -l, --log-level=<LOG_LEVEL>   Logging level, valid values: error, [warning],");
	CERR("                                  notice, info, debug.")
	CERR("    -m, --mtu=<MTU>               Specify the interface MTU of between {} and", SETH_MIN_MTU_SIZE);
	CERR("                                  {} (default is 1500)", SETH_MAX_MTU_SIZE);
	CERR("    -t, --txsize=<TSIZE>          Specify the maximum transmissions packet size");
	CERR("                                  of between {} and {} (default is 1500)", SETH_MIN_TXSIZE, SETH_MAX_TXSIZE);
	CERR("    -s, --src=<SOURCE>            Specify the source IPv4/IPv6 address");
	CERR("                                  (mandatory)");
	CERR("    -d, --dst=<DESTINATION>       Specify the destination IPv4/IPv6 address");
	CERR("                                  (mandatory)");
	CERR("    -p, --port=<PORT>             Specify the UDP port, between 1 and 65535");
	CERR("                                  (default is 58023)");
	CERR("    -i, --ifname=<IFNAME>         Specify interface name to use up to {}", IFNAMSIZ);
	CERR("                                  characters (default is \"{}\")", SETH_DEFAULT_TUNNEL_NAME);
	CERR("");
	
}

int main(int argc, char *argv[]) {
	int c;
	int option_index = 0;
	accl::LogLevel log_level = accl::LogLevel::WARNING;
	int mtu_value = 1500;
	int txsize_value = 1500;
	int port_value = 58023;
	char *src_value = NULL;
	char *dst_value = NULL;
	std::string ifname_value = SETH_DEFAULT_TUNNEL_NAME;

	CERR("Super Ethernet Tunnel v{} - Copyright (c) 2023, AllWorldIT.", VERSION);
	CERR("");

	while (1) {
		c = getopt_long(argc, argv, "vhl:m:s:r:d:p:i:", long_options, &option_index);

		// Detect end of the options
		if (c == -1) {
			break;
		}

		switch (c) {
		case 'v':
			std::cout << std::format("Version: {}", VERSION) << std::endl;
			return 0;
		case 'h':
			CERR("");
			print_help();
			return 0;
		case 'l': {
			std::string log_level_opt(optarg);
			if (accl::logLevelMap.find(log_level_opt) != accl::logLevelMap.end()) {
				log_level = accl::logLevelMap[log_level_opt];
			} else {
				CERR("ERROR: Invalid log level '{}'.", log_level_opt);
				return 1;
			}
			break;
		}
		case 'm':
			mtu_value = atoi(optarg);
			if (mtu_value < SETH_MIN_MTU_SIZE || mtu_value > SETH_MAX_MTU_SIZE) {
				CERR("ERROR: Invalid MTU value. It should be between {} and {}.", SETH_MIN_MTU_SIZE, SETH_MAX_MTU_SIZE);
				return 1;
			}
			break;
		case 't':
			txsize_value = atoi(optarg);
			if (txsize_value < SETH_MIN_TXSIZE || txsize_value > SETH_MAX_TXSIZE) {
				CERR("ERROR: Invalid TX_SIZE value. It should be between {} and {}.", SETH_MIN_TXSIZE, SETH_MAX_TXSIZE);
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
				CERR("Invalid port value. It should be between 1 and 65535.");
				return 1;
			}
			break;
		case 'i': // Handling the ifname option
			if (strlen(optarg) >= IFNAMSIZ) {
				CERR("ERROR: Invalid interface name. It should be less than {} characters.", IFNAMSIZ);
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
		CERR("ERROR: Both --src (-s) and --dst (-d) options are mandatory.");
		CERR("");
		print_help();
		return 1;
	}

	// Convert local address to a IPv6 sockaddr
	struct in6_addr src_addr;
	char src_addr_str[INET6_ADDRSTRLEN];
	if (to_sin6addr(src_value, &src_addr)) {
		if (inet_ntop(AF_INET6, &src_addr, src_addr_str, sizeof(src_addr_str)) == NULL) {
			CERR("ERROR: Failed to convert source address '{}' to IPv6 address: {}", src_value, strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		CERR("ERROR: Failed to convert source address '{}' to IPv6 address", src_value);
		exit(EXIT_FAILURE);
	}

	// Convert remote address to a IPv6 sockaddr
	struct in6_addr dst_addr;
	char dst_addr_str[INET6_ADDRSTRLEN];
	if (to_sin6addr(dst_value, &dst_addr)) {
		if (inet_ntop(AF_INET6, &dst_addr, dst_addr_str, sizeof(dst_addr_str)) == NULL) {
			CERR("ERROR: Failed to convert destination address '{}' to IPv6 address: {}", dst_value, strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		CERR("ERROR: Failed to convert source address '{}' to IPv6 address", dst_value);
		exit(EXIT_FAILURE);
	}

	// Set our log level
	accl::logger.setLogLevel(log_level);
	LOG_INFO("Logging level set to {}", accl::logger.getLogLevelString());

	CERR("Interface...: {}", ifname_value.c_str());
	CERR("Source......: {}", src_addr_str);
	CERR("Destination.: {}", dst_addr_str);
	CERR("UDP Port....: {}", port_value);
	CERR("MTU.........: {}", mtu_value);
	CERR("TX Size.....: {}", txsize_value);
	CERR("");

	start_set(ifname_value, &src_addr, &dst_addr, port_value, mtu_value, txsize_value);

	return 0;
}
