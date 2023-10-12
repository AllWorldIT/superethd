#pragma once

extern "C" {
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdint.h>
}

#include <stdexcept>
#include <string>

#include "common.hpp"
#include "endian.hpp"

/*
 * @brief Packet not currently supported exception.
 *
 */
class PacketNotSupportedEception : public std::runtime_error {
	public:
		PacketNotSupportedEception(const std::string &message) : std::runtime_error(message) {}
};

/*
 * @brief Packet malformed exception.
 *
 */
class PacketMalformedEception : public std::runtime_error {
	public:
		PacketMalformedEception(const std::string &message) : std::runtime_error(message) {}
};

#define SETH_PACKET_ETHERNET_HEADER_LEN 14
#define SETH_PACKET_ETHERNET_MAC_LEN 6

#define SETH_PACKET_TYPE_ETHERNET_IPV4 0x0800
#define SETH_PACKET_TYPE_ETHERNET_ARP 0x0806
#define SETH_PACKET_TYPE_ETHERNET_VLAN 0x8100
#define SETH_PACKET_TYPE_ETHERNET_IPV6 0x86DD

#define SETH_PACKET_IP_HEADER_LEN 1

#define SETH_PACKET_IP_VERSION_IPV4 0x4
#define SETH_PACKET_IP_VERSION_IPV6 0x6

#define SETH_PACKET_IP_PROTOCOL_ICMP4 1
#define SETH_PACKET_IP_PROTOCOL_TCP 6
#define SETH_PACKET_IP_PROTOCOL_UDP 17
#define SETH_PACKET_IP_PROTOCOL_ICMP6 58

#define SETH_PACKET_IPV4_HEADER_LEN 20
#define SETH_PACKET_IPV4_IP_LEN 4

#define SETH_PACKET_ICMP_HEADER_LEN 4
#define SETH_PACKET_ICMP_ECHO_HEADER_LEN SETH_PACKET_ICMP_HEADER_LEN + 4
#define SETH_PACKET_UDP_HEADER_LEN 8
#define SETH_PACKET_TCP_HEADER_LEN 20

#define SETH_PACKET_ICMPV4_TYPE_ECHO_REPLY 0
#define SETH_PACKET_ICMPV4_TYPE_ECHO_REQUEST 8

typedef struct __seth_packed {
		uint8_t dst_mac[SETH_PACKET_ETHERNET_MAC_LEN]; // Destination MAC address
		uint8_t src_mac[SETH_PACKET_ETHERNET_MAC_LEN]; // Source MAC address
		seth_be16_t ethertype;						   // Ethertype field to indicate the protocol
		uint8_t payload[];							   // Variable length payload
} ethernet_packet_t;

// IP header definition
typedef struct __seth_packed {
#if SETH_BYTE_ORDER == SETH_BIG_ENDIAN
		uint8_t version : 4; // IP version (should be 4 for IPv4)
		uint8_t unused : 4;	 // Unused bits
#else
		uint8_t unused : 4;	  // Unused bits
		uint8_t version : 4;  // IP version (should be 4 for IPv4)
#endif
} ip_header_t;

typedef struct __seth_packed {
		seth_be16_t src_port; // Source port
		seth_be16_t dst_port; // Destination port
		seth_be16_t length;	  // Length of the UDP header and payload
		seth_be16_t checksum; // Checksum
		uint8_t payload[];	  // Optional: Variable length field for data payload
} udp_header_t;

typedef struct __seth_packed {
		uint8_t type;	   // Type
		uint8_t code;	   // Code
		uint16_t checksum; // Checksum
		uint8_t payload[]; // Payload
} icmp_header_t;

typedef struct __seth_packed {
		uint8_t type;			// Type
		uint8_t code;			// Code
		seth_be16_t checksum;	// Checksum
		seth_be16_t identifier; // Identifier
		seth_be16_t sequence;	// Sequence
		uint8_t payload[];		// Payload
} icmp_echo_header_t;

typedef struct __seth_packet {
		uint8_t cwr : 1;
		uint8_t ece : 1;
		uint8_t urg : 1;
		uint8_t ack : 1;
		uint8_t psh : 1;
		uint8_t rst : 1;
		uint8_t syn : 1;
		uint8_t fin : 1;
} tcp_options_header_t;

typedef struct __seth_packed {
		uint16_t src_port; // Source port
		uint16_t dst_port; // Destination port

		uint32_t sequence; // Sequence

		uint32_t ack; // Acknowledgment if ACK set

#if SETH_BYTE_ORDER == SETH_BIG_ENDIAN
		uint8_t data_offset : 4; // Data offset
		uint8_t reserved : 4;	 // Reserved bits
#else
		uint8_t reserved : 4; // Reserved bits
		uint8_t ihl : 4;	  // Data offset
#endif

		tcp_options_header_t options; // TCP optoins
		uint16_t window;			  // Window size

		uint16_t checksum; // Checksum
		uint16_t urgent;   // Urgent pointer

		uint8_t payload[]; // Variable length payload
} tcp_header_t;

// IPv4 header definition
typedef struct __seth_packed {
#if SETH_BYTE_ORDER == SETH_BIG_ENDIAN
		uint8_t version : 4; // IP version (should be 4 for IPv4)
		uint8_t ihl : 4;	 // Internet Header Length (header length in 32-bit words)
#else
		uint8_t ihl : 4;	  // Internet Header Length (header length in 32-bit words)
		uint8_t version : 4;  // IP version (should be 4 for IPv4)
#endif
		uint8_t dscp : 6;		  // DSCP
		uint8_t ecn : 2;		  // DSCP
		seth_be16_t total_length; // Total packet length (header + data)
		seth_be16_t id;			  // Identification
		seth_be16_t frag_off;	  // Flags (3 bits) and Fragment Offset (13 bits)
		uint8_t ttl;			  // Time to Live
		uint8_t protocol;		  // Protocol (e.g., TCP, UDP)
		seth_be16_t checksum;	  // Header checksum
		uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN];
		uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN];
		// Options and padding might follow here if IHL > 5
		uint8_t payload[]; // Variable length payload
} ipv4_header_t;

// IPv6 header definition
typedef struct __seth_packed {
		uint8_t version : 4;		// IP version (should be 6 for IPv6)
		uint8_t traffic_class;		// Traffic Class
		uint32_t flow_label : 20;	// Flow Label
		seth_be16_t payload_length; // Length of the payload
		uint8_t next_header;		// Identifies the type of the next header
		uint8_t hop_limit;			// Similar to TTL in IPv4
		uint8_t src_addr[16];		// Source IP address
		uint8_t dst_addr[16];		// Destination IP address
		uint8_t payload[];			// Variable length payload
} ipv6_header_t;

// Union for IP headers
typedef union __seth_packed {
		ip_header_t ip;
		ipv4_header_t ipv4;
		ipv6_header_t ipv6;
} ip_header_union_t;

void add_packet_ethernet_header(ethernet_packet_t **pkt, uint16_t *pkt_size, uint8_t dst_mac[ETH_ALEN], uint8_t src_mac[ETH_ALEN],
								uint16_t ethertype);

void add_packet_ipv4_header(ethernet_packet_t **pkt, uint16_t *pkt_size, uint16_t id, uint8_t src_addr[SETH_PACKET_IPV4_IP_LEN],
							uint8_t dst_addr[SETH_PACKET_IPV4_IP_LEN]);

void add_packet_udp_header(ethernet_packet_t **pkt, uint16_t *pkt_size, uint16_t src_port, uint16_t dst_port);
void add_packet_icmp_header(ethernet_packet_t **pkt, uint16_t *pkt_size, uint8_t type, uint8_t code);
void add_packet_icmp_echo_header(ethernet_packet_t **pkt, uint16_t *pkt_size, uint16_t identifier, uint16_t sequence);
void add_payload(ethernet_packet_t **pkt, uint16_t *pkt_size, uint8_t *payload, uint16_t payload_size);
void set_length(ethernet_packet_t **pkt, uint16_t pkt_size);
void set_checksums(ethernet_packet_t **pkt, uint16_t pkt_size);

uint16_t compute_ipv4_header_checksum(ethernet_packet_t *pkt, uint16_t length);
uint16_t compute_ipv4_udp_checksum(ethernet_packet_t *pkt, uint16_t length);
uint16_t compute_ipv4_icmp_checksum(ethernet_packet_t *pkt, uint16_t length);

int is_valid_ethernet(const ethernet_packet_t *packet, uint16_t size);
int is_valid_ethernet_ip(const ethernet_packet_t *packet, uint16_t size);
int is_valid_ethernet_ipv4(const ethernet_packet_t *packet, uint16_t size);
int is_valid_ethernet_ipv4_udp(const ethernet_packet_t *packet, uint16_t size);
int is_valid_ethernet_ipv4_icmp(const ethernet_packet_t *packet, uint16_t size);
