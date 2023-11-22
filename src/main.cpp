/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <string>

#include "Codec.hpp"
#include "common.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "libaccl/Logger.hpp"
#include "superethd.hpp"
#include "util.hpp"
#include <arpa/inet.h>
#include <getopt.h>
#include <net/if.h>
#include <string.h>

static struct option long_options[] = {
	{"version", no_argument, 0, 'v'},	{"help", no_argument, 0, 'h'},		  {"ifname", required_argument, 0, 'i'},
	{"src", required_argument, 0, 's'}, {"dst", required_argument, 0, 'd'},	  {"port", required_argument, 0, 'p'},
	{"mtu", required_argument, 0, 'm'}, {"tsize", required_argument, 0, 't'}, {0, 0, 0, 0}};

void print_help() {
	std::cerr << "Usage:" << std::endl;
	std::cerr << "    -v,    Print version information" << std::endl;
	std::cerr << std::endl;
	std::cerr << "    -h, --help                    Print this help" << std::endl;
	std::cerr << "    -l, --log-level=<LOG_LEVEL>   Logging level, valid values: error, warning," << std::endl;
	std::cerr << std::format("                                  notice, info, debug (default is \"{}\")",
							 accl::logger.getLogLevelDefaultString())
			  << std::endl;
	std::cerr << std::format("    -m, --mtu=<MTU>               Specify the interface MTU of between {} and", SETH_MIN_MTU_SIZE)
			  << std::endl;
	std::cerr << std::format("                                  {} (default is 1500)", SETH_MAX_MTU_SIZE) << std::endl;
	std::cerr << "    -t, --txsize=<TSIZE>          Specify the maximum transmissions packet size" << std::endl;
	std::cerr << std::format("                                  of between {} and {} (default is 1500)", SETH_MIN_TXSIZE,
							 SETH_MAX_TXSIZE)
			  << std::endl;
	std::cerr << "    -s, --src=<SOURCE>            Specify the source IPv4/IPv6 address" << std::endl;
	std::cerr << "                                  (mandatory)" << std::endl;
	std::cerr << "    -d, --dst=<DESTINATION>       Specify the destination IPv4/IPv6 address" << std::endl;
	std::cerr << "                                  (mandatory)" << std::endl;
	std::cerr << "    -p, --port=<PORT>             Specify the UDP port, between 1 and 65535" << std::endl;
	std::cerr << "                                  (default is 58023)" << std::endl;
	std::cerr << std::format("    -i, --ifname=<IFNAME>         Specify interface name to use up to {}", IFNAMSIZ) << std::endl;
	std::cerr << std::format("                                  characters (default is \"{}\")", SETH_DEFAULT_TUNNEL_NAME)
			  << std::endl;
	std::cerr << "    -c, --compression=<COMPR>     Specify compression algorithm to use, valid" << std::endl;
	std::cerr << std::format("                                  values: none, lz4, zstd (default: \"{}\")",
							 PacketHeaderOptionFormatTypeToString(SETH_DEFAULT_PACKET_FORMAT))
			  << std::endl;
	std::cerr << std::endl;
}

int main(int argc, char *argv[]) {
	int c;
	int option_index = 0;
	int mtu_value = 1500;
	int txsize_value = 1500;
	int port_value = 58023;
	char *src_value = NULL;
	char *dst_value = NULL;
	std::string ifname_value = SETH_DEFAULT_TUNNEL_NAME;
	PacketHeaderOptionFormatType packet_format = PacketHeaderOptionFormatType::COMPRESSED_LZ4;

	std::cerr << std::format("Super Ethernet Tunnel v{} - Copyright (c) 2023, AllWorldIT.", VERSION) << std::endl;
	std::cerr << std::endl;

	while (1) {
		c = getopt_long(argc, argv, "vhl:m:t:s:r:d:p:i:c:", long_options, &option_index);

		// Detect end of the options
		if (c == -1) {
			break;
		}

		switch (c) {
		case 'v':
			std::cout << std::format("Version: {}", VERSION) << std::endl;
			return 0;
		case 'h':
			std::cerr << std::endl;
			print_help();
			return 0;
		case 'l': {
			if (!accl::logger.setLogLevel(optarg)) {
				std::cerr << std::format("ERROR: Invalid log level '{}'.", optarg) << std::endl;
				return 1;
			}
			break;
		}
		case 'm':
			mtu_value = atoi(optarg);
			if (mtu_value < SETH_MIN_MTU_SIZE || mtu_value > SETH_MAX_MTU_SIZE) {
				std::cerr << std::format("ERROR: Invalid MTU value. It should be between {} and {}.", SETH_MIN_MTU_SIZE,
										 SETH_MAX_MTU_SIZE)
						  << std::endl;
				return 1;
			}
			break;
		case 't':
			txsize_value = atoi(optarg);
			if (txsize_value < SETH_MIN_TXSIZE || txsize_value > SETH_MAX_TXSIZE) {
				std::cerr << std::format("ERROR: Invalid TX_SIZE value. It should be between {} and {}.", SETH_MIN_TXSIZE,
										 SETH_MAX_TXSIZE)
						  << std::endl;
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
				std::cerr << "Invalid port value. It should be between 1 and 65535." << std::endl;
				return 1;
			}
			break;
		case 'i': // Handling the ifname option
			if (strlen(optarg) >= IFNAMSIZ) {
				std::cerr << std::format("ERROR: Invalid interface name. It should be less than {} characters.", IFNAMSIZ)
						  << std::endl;
				return 1;
			}
			ifname_value.assign(optarg);
			break;
		case 'c': // Handling the compression option
			if (strcmp(optarg, "none") == 0) {
				packet_format = PacketHeaderOptionFormatType::NONE;
				break;
			} else if (strcmp(optarg, "lz4") == 0) {
				packet_format = PacketHeaderOptionFormatType::COMPRESSED_LZ4;
				break;
			} else if (strcmp(optarg, "zstd") == 0) {
				packet_format = PacketHeaderOptionFormatType::COMPRESSED_ZSTD;
				break;
			} else {
				std::cerr << std::format("ERROR: Invalid compression algorithm '{}'.", optarg) << std::endl;
				return 1;
			}
		case '?':
			return 1;
		default:
			abort();
		}
	}

	// Check if mandatory options src and dst are provided
	if (!src_value || !dst_value) {
		std::cerr << "ERROR: Both --src (-s) and --dst (-d) options are mandatory." << std::endl;
		std::cerr << std::endl;
		print_help();
		return 1;
	}

	// Convert local address to a IPv6 sockaddr
	struct in6_addr src_addr;
	char src_addr_str[INET6_ADDRSTRLEN];
	if (to_sin6addr(src_value, &src_addr)) {
		if (inet_ntop(AF_INET6, &src_addr, src_addr_str, sizeof(src_addr_str)) == NULL) {
			std::cerr << std::format("ERROR: Failed to convert source address '{}' to IPv6 address: {}", src_value, strerror(errno))
					  << std::endl;
			exit(EXIT_FAILURE);
		}
	} else {
		std::cerr << std::format("ERROR: Failed to convert source address '{}' to IPv6 address", src_value) << std::endl;
		exit(EXIT_FAILURE);
	}

	// Convert remote address to a IPv6 sockaddr
	struct in6_addr dst_addr;
	char dst_addr_str[INET6_ADDRSTRLEN];
	if (to_sin6addr(dst_value, &dst_addr)) {
		if (inet_ntop(AF_INET6, &dst_addr, dst_addr_str, sizeof(dst_addr_str)) == NULL) {
			std::cerr << std::format("ERROR: Failed to convert destination address '{}' to IPv6 address: {}", dst_value,
									 strerror(errno))
					  << std::endl;

			exit(EXIT_FAILURE);
		}
	} else {
		std::cerr << std::format("ERROR: Failed to convert source address '{}' to IPv6 address", dst_value) << std::endl;
		exit(EXIT_FAILURE);
	}

	// Set our log level
	std::cerr << std::format("Logging level set to {}", accl::logger.getLogLevelString()) << std::endl;
	std::cerr << std::endl;

	std::cerr << std::format("Interface...: {}", ifname_value.c_str()) << std::endl;
	std::cerr << std::format("Source......: {}", src_addr_str) << std::endl;
	std::cerr << std::format("Destination.: {}", dst_addr_str) << std::endl;
	std::cerr << std::format("UDP Port....: {}", port_value) << std::endl;
	std::cerr << std::format("MTU.........: {}", mtu_value) << std::endl;
	std::cerr << std::format("TX Size.....: {}", txsize_value) << std::endl;
	std::cerr << std::endl;

	start_set(ifname_value, &src_addr, &dst_addr, port_value, mtu_value, txsize_value, packet_format);

	return 0;
}
