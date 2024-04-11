/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "codec.hpp"
#include "common.hpp"
#include "config.hpp"
#include "libaccl/logger.hpp"
#include "superethd.hpp"
#include "util.hpp"
#include <arpa/inet.h>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <format>
#include <getopt.h>
#include <iostream>
#include <net/if.h>
#include <string.h>
#include <string>

static struct option long_options[] = {{"version", no_argument, 0, 'v'},
									   {"help", no_argument, 0, 'h'},
									   {"config-file", required_argument, 0, 'c'},
									   {"ifname", required_argument, 0, 'i'},
									   {"src", required_argument, 0, 's'},
									   {"dst", required_argument, 0, 'd'},
									   {"port", required_argument, 0, 'p'},
									   {"mtu", required_argument, 0, 'm'},
									   {"tsize", required_argument, 0, 't'},
									   {"compression", required_argument, 0, 'a'},
									   {0, 0, 0, 0}};

void print_help() {
	std::cerr << "Usage:" << std::endl;
	std::cerr << "    -v,    Print version information" << std::endl;
	std::cerr << "    -h, --help                    Print this help" << std::endl;
	std::cerr << "    -c, --config-file             Specify the configuration file to use" << std::endl;
	std::cerr << std::format("                                  (default is \"{}\")", DEFAULT_CONFIGURATION_FILE) << std::endl;
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
	std::cerr << "    -a, --compression=<COMPR>     Specify compression algorithm to use, valid" << std::endl;
	std::cerr << std::format("                                  values: none, lz4, zstd (default: \"{}\")",
							 PacketHeaderOptionFormatTypeToString(SETH_DEFAULT_PACKET_FORMAT))
			  << std::endl;
	std::cerr << std::endl;
}

int main(int argc, char *argv[]) {
	// Configuration options and defaults
	std::string cfg_config_file{DEFAULT_CONFIGURATION_FILE};
	std::string cfg_log_level;
	int cfg_mtu{1500};
	int cfg_txsize{1500};
	int cfg_tunnel_port{58203};
	std::string cfg_tunnel_src{};
	std::string cfg_tunnel_dst{};
	std::string cfg_ifname{SETH_DEFAULT_TUNNEL_NAME};
	std::string cfg_packet_format_str{"lz4"};
	PacketHeaderOptionFormatType cfg_packet_format;

	std::cerr << std::format("Super Ethernet Tunnel v{} - Copyright (c) 2023, AllWorldIT.", VERSION) << std::endl;
	std::cerr << std::endl;

	{
		// Command line option and config file processing variables
		int c;
		int option_index{0};
		std::string cmdline_config_file;
		std::string cmdline_log_level, conffile_log_level;
		int cmdline_mtu{0}, conffile_mtu{0};
		int cmdline_txsize{0}, conffile_txsize{0};
		int cmdline_tunnel_port{0}, conffile_tunnel_port{0};
		std::string cmdline_tunnel_src, conffile_tunnel_src;
		std::string cmdline_tunnel_dst, conffile_tunnel_dst;
		std::string cmdline_ifname, conffile_ifname;
		std::string cmdline_packet_format, conffile_packet_format;

		while (1) {
			c = getopt_long(argc, argv, "vhc:l:m:t:s:r:d:p:i:a:", long_options, &option_index);

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
			case 'c':
				if (!std::filesystem::exists(optarg)) {
					std::cerr << std::format("ERROR: Configuration file '{}' does not exist.", optarg) << std::endl;
					return 1;
				}
				cmdline_config_file.assign(optarg);
				break;
			case 'l': {
				cmdline_log_level.assign(optarg);
				break;
			}
			case 'm':
				cmdline_mtu = atoi(optarg);
				break;
			case 't':
				cmdline_txsize = atoi(optarg);
				break;
			case 's':
				cmdline_tunnel_src.assign(optarg);
				break;
			case 'd':
				cmdline_tunnel_dst.assign(optarg);
				break;
			case 'p':
				cmdline_tunnel_port = atoi(optarg);
				break;
			case 'i':
				cmdline_ifname.assign(optarg);
				break;
			case 'a': // Handling the compression option
				cmdline_packet_format.assign(optarg);
				break;
			case '?':
				return 1;
			default:
				abort();
			}
		}

		// Check if the configuration file exists if it was specified on the command line
		if (cmdline_config_file.length() > 0) {
			if (!std::filesystem::exists(cmdline_config_file)) {
				std::cerr << std::format("ERROR: Configuration file '{}' does not exist.", cmdline_config_file) << std::endl;
				return 1;
			}
			cfg_config_file.assign(cmdline_config_file);
		}
		// If we have a config file, parse it
		if (std::filesystem::exists(cfg_config_file)) {
			std::cerr << std::format("Loading configuration file '{}'", cfg_config_file) << std::endl;
			// Parse configuration file and check for errors
			boost::property_tree::ptree pt;
			try {
				boost::property_tree::ini_parser::read_ini(cfg_config_file, pt);
			} catch (boost::property_tree::ini_parser::ini_parser_error &e) {
				std::cerr << std::format("ERROR: Failed to parse configuration file '{}': {}", cfg_config_file, e.what())
						  << std::endl;
				return 1;
			}
			// Check if the log level is available in the config
			try {
				conffile_log_level = pt.get<std::string>("loglevel").c_str();
			} catch (boost::property_tree::ptree_bad_path &e) {
				// Ignore if its not found
			}
			// Check if the MTU is available in the config
			try {
				conffile_mtu = pt.get<int>("mtu");
			} catch (boost::property_tree::ptree_bad_path &e) {
				// Ignore if its not found
			}
			// Check if the TX_SIZE is available in the config
			try {
				conffile_txsize = pt.get<int>("txsize");
			} catch (boost::property_tree::ptree_bad_path &e) {
				// Ignore if its not found
			}
			// Check if the source address is available in the config
			try {
				conffile_tunnel_src = pt.get<std::string>("source").c_str();
			} catch (boost::property_tree::ptree_bad_path &e) {
				// Ignore if its not found
			}
			// Check if the destination address is available in the config
			try {
				conffile_tunnel_dst = pt.get<std::string>("destination").c_str();
			} catch (boost::property_tree::ptree_bad_path &e) {
				// Ignore if its not found
			}
			// Check if the port is available in the config
			try {
				conffile_tunnel_port = pt.get<int>("port");
			} catch (boost::property_tree::ptree_bad_path &e) {
				// Ignore if its not found
			}
			// Check if the interface name is available in the config
			try {
				conffile_ifname = pt.get<std::string>("interface").c_str();
			} catch (boost::property_tree::ptree_bad_path &e) {
				// Ignore if its not found
			}
			// Check if the compression algorithm is available in the config
			try {
				conffile_packet_format = pt.get<std::string>("compression").c_str();
			} catch (boost::property_tree::ptree_bad_path &e) {
				// Ignore if its not found
			}
		}

		// Work out what log level we're using,
		cfg_log_level = cmdline_log_level.length() > 0 ? cmdline_log_level : conffile_log_level;
		if (cfg_log_level.length() > 0) {
			if (!accl::logger.setLogLevel(cfg_log_level)) {
				std::cerr << std::format("ERROR: Invalid log level '{}'.", cfg_log_level) << std::endl;
				return 1;
			}
		}

		// Work out what MTU we're using
		if (cmdline_mtu > 0) {
			cfg_mtu = cmdline_mtu;
		} else if (conffile_mtu > 0) {
			cfg_mtu = conffile_mtu;
		}
		if (cfg_mtu < SETH_MIN_MTU_SIZE || cfg_mtu > SETH_MAX_MTU_SIZE) {
			std::cerr << std::format("ERROR: Invalid MTU value. It should be between {} and {}.", SETH_MIN_MTU_SIZE,
									 SETH_MAX_MTU_SIZE)
					  << std::endl;
			return 1;
		}

		// Work out what TX_SIZE we're using
		if (cmdline_txsize > 0) {
			cfg_txsize = cmdline_txsize;
		} else if (conffile_txsize > 0) {
			cfg_txsize = conffile_txsize;
		}
		if (cfg_txsize < SETH_MIN_TXSIZE || cfg_txsize > SETH_MAX_TXSIZE) {
			std::cerr << std::format("ERROR: Invalid TX_SIZE value. It should be between {} and {}.", SETH_MIN_TXSIZE,
									 SETH_MAX_TXSIZE)
					  << std::endl;
			return 1;
		}

		// Work out what tunnel src we're using
		if (cmdline_tunnel_src.length() > 0) {
			cfg_tunnel_src = cmdline_tunnel_src;
		} else if (conffile_tunnel_src.length() > 0) {
			cfg_tunnel_src = conffile_tunnel_src;
		}
		// Work out what tunnel dst we're using
		if (cmdline_tunnel_dst.length() > 0) {
			cfg_tunnel_dst = cmdline_tunnel_dst;
		} else if (conffile_tunnel_dst.length() > 0) {
			cfg_tunnel_dst = conffile_tunnel_dst;
		}
		// Check if mandatory options src and dst are provided
		if (!(cfg_tunnel_src.length() > 0 && cfg_tunnel_dst.length() > 0)) {
			std::cerr << "ERROR: Tunnel source and destination are mandatory and must be present" << std::endl;
			std::cerr << "       in either the config file or on the command line." << std::endl;
			return 1;
		}

		// Work out what tunnel port we're uwing
		if (cmdline_tunnel_port > 0) {
			cfg_tunnel_port = cmdline_tunnel_port;
		} else if (conffile_tunnel_port > 0) {
			cfg_tunnel_port = conffile_tunnel_port;
		}
		if (cfg_tunnel_port < 1 || cfg_tunnel_port > 65535) {
			std::cerr << std::format("ERROR: Invalid tunnel port value '{}'. It should be between 1 and 65535.", cfg_tunnel_port)
					  << std::endl;
			return 1;
		}

		// Work out what interface name we're using
		if (cmdline_ifname.length() > 0) {
			cfg_ifname = cmdline_ifname;
		} else if (conffile_ifname.length() > 0) {
			cfg_ifname = conffile_ifname;
		}
		if (cfg_ifname.length() >= IFNAMSIZ) {
			std::cerr << std::format("ERROR: Invalid interface name. It should be less than {} characters.", IFNAMSIZ) << std::endl;
			return 1;
		}

		// Work out what compression algorithm we're using
		if (cmdline_packet_format.length() > 0) {
			cfg_packet_format_str = cmdline_packet_format;
		} else if (conffile_packet_format.length() > 0) {
			cfg_packet_format_str = conffile_packet_format;
		}
		if (cfg_packet_format_str.compare("none") == 0) {
			cfg_packet_format = PacketHeaderOptionFormatType::NONE;
		} else if (cfg_packet_format_str.compare("lz4") == 0) {
			cfg_packet_format = PacketHeaderOptionFormatType::COMPRESSED_LZ4;

		} else if (cfg_packet_format_str.compare("zstd") == 0) {
			cfg_packet_format = PacketHeaderOptionFormatType::COMPRESSED_ZSTD;
		} else {
			std::cerr << std::format("ERROR: Invalid compression algorithm '{}'.", cfg_packet_format_str) << std::endl;
			return 1;
		}
	}

	/*
	 * Further config processing, once we've determined what we're using
	 */

	// Convert local address to a IPv6 sockaddr
	struct in6_addr src_addr;
	char src_addr_str[INET6_ADDRSTRLEN];
	if (to_sin6addr(cfg_tunnel_src.data(), &src_addr)) {
		if (inet_ntop(AF_INET6, &src_addr, src_addr_str, sizeof(src_addr_str)) == NULL) {
			std::cerr << std::format("ERROR: Failed to convert source address '{}' to IPv6 address: {}", cfg_tunnel_src,
									 strerror(errno))
					  << std::endl;
			exit(EXIT_FAILURE);
		}
	} else {
		std::cerr << std::format("ERROR: Failed to convert source address '{}' to IPv6 address", cfg_tunnel_src) << std::endl;
		exit(EXIT_FAILURE);
	}

	// Convert remote address to a IPv6 sockaddr
	struct in6_addr dst_addr;
	char dst_addr_str[INET6_ADDRSTRLEN];
	if (to_sin6addr(cfg_tunnel_dst.data(), &dst_addr)) {
		if (inet_ntop(AF_INET6, &dst_addr, dst_addr_str, sizeof(dst_addr_str)) == NULL) {
			std::cerr << std::format("ERROR: Failed to convert destination address '{}' to IPv6 address: {}", cfg_tunnel_dst,
									 strerror(errno))
					  << std::endl;

			exit(EXIT_FAILURE);
		}
	} else {
		std::cerr << std::format("ERROR: Failed to convert source address '{}' to IPv6 address", cfg_tunnel_dst) << std::endl;
		exit(EXIT_FAILURE);
	}

	// Set our log level
	std::cerr << std::format("Logging level set to {}", accl::logger.getLogLevelString()) << std::endl;
	std::cerr << std::endl;

	std::cerr << std::format("Interface...: {}", cfg_ifname) << std::endl;
	std::cerr << std::format("Source......: {}", src_addr_str) << std::endl;
	std::cerr << std::format("Destination.: {}", dst_addr_str) << std::endl;
	std::cerr << std::format("UDP Port....: {}", cfg_tunnel_port) << std::endl;
	std::cerr << std::format("MTU.........: {}", cfg_mtu) << std::endl;
	std::cerr << std::format("TX Size.....: {}", cfg_txsize) << std::endl;
	std::cerr << std::endl;

	start_set(cfg_ifname, &src_addr, &dst_addr, cfg_tunnel_port, cfg_mtu, cfg_txsize, cfg_packet_format);

	return 0;
}
