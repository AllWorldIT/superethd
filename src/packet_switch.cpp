/*
 * SPDX-FileCopyrightText: 2023-2024 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "packet_switch.hpp"
#include "common.hpp"
#include "exceptions.hpp"
#include "fdb_entry.hpp"
#include "libaccl/buffer_pool.hpp"
#include "libaccl/logger.hpp"
#include "libsethnetkit/ethernet_packet.hpp"
#include "packet_buffer.hpp"
#include "threads.hpp"
#include "util.hpp"
#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <memory>
#include <sched.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

PacketSwitch::PacketSwitch(const std::string ifname, int mtu, int tx_size, PacketHeaderOptionFormatType packet_format,
						   std::shared_ptr<struct sockaddr_storage> src_addr,
						   std::vector<std::shared_ptr<struct sockaddr_storage>> dst_addrs, int port) {
	// MTU
	this->mtu = mtu;
	if (this->mtu > SETH_MAX_MTU_SIZE) {
		throw SuperEthernetTunnelConfigException("Maximum MTU is " + std::to_string(SETH_MAX_MTU_SIZE) + "!");
	}
	if (this->mtu < SETH_MIN_MTU_SIZE) {
		throw SuperEthernetTunnelConfigException("Minimum MTU is " + std::to_string(SETH_MIN_MTU_SIZE) + "!");
	}

	// Maximum TX size
	this->tx_size = tx_size;
	if (this->tx_size > SETH_MAX_TXSIZE) {
		throw SuperEthernetTunnelConfigException("Maximum TX_SIZE is " + std::to_string(SETH_MAX_TXSIZE) + "!");
	}
	if (this->tx_size < SETH_MIN_TXSIZE) {
		throw SuperEthernetTunnelConfigException("Minimum TX_SIZE is " + std::to_string(SETH_MIN_TXSIZE) + "!");
	}
	if (this->tx_size > this->mtu) {
		throw SuperEthernetTunnelConfigException("TX_SIZE " + std::to_string(this->tx_size) + " cannot be greater than MTU is " +
												 std::to_string(this->mtu) + "!");
	}

	// Set up packet format
	this->packet_format = packet_format;

	// Set up our source address
	this->src_addr = src_addr;

	// Create TAP interface
	this->tap_interface = std::make_unique<TAPInterface>(ifname);
	this->tap_interface->setMTU(this->mtu);

	// Initialize the udp socket to 0 to indicate its not been setup
	this->udp_socket = 0;

	// Set port
	this->port = port;

	// Create our FDB
	this->fdb = std::make_shared<FDB>();
	this->fdb_expire_time = 300;

	// Get maximum ethernet frame size we can get
	this->l2mtu = get_l2mtu_from_mtu(mtu);

	// Add on 10% of the buffer size to cater for compression overhead
	int bufferSize = this->l2mtu + (this->l2mtu / 10);

	// Available RX and TX buffer pools
	this->available_rx_buffer_pool =
		std::make_shared<accl::BufferPool<PacketBuffer>>(bufferSize, SETH_BUFFER_COUNT * dst_addrs.size());
	this->available_tx_buffer_pool =
		std::make_shared<accl::BufferPool<PacketBuffer>>(bufferSize, SETH_BUFFER_COUNT * dst_addrs.size());
	this->tap_write_pool = std::make_shared<accl::BufferPool<PacketBuffer>>(bufferSize);

	// Create UDP socket
	this->_create_udp_socket();

	// Loop with dst_addrs and crate RemoteNodes to be added to our remote_nodes map
	for (auto &dst_addr : dst_addrs) {
		// Create remote node
		auto remote_node = std::make_shared<RemoteNode>(this->udp_socket, dst_addr, this->tx_size, this->l2mtu, bufferSize,
														this->packet_format, this->tap_write_pool, this->available_rx_buffer_pool,
														this->available_tx_buffer_pool, &this->stop_flag);
		// Add to our remote nodes map
		this->remote_nodes[remote_node->getNodeKey()] = remote_node;
	}
}

PacketSwitch::~PacketSwitch() {
	// FIXME
	LOG_WARNING("NKDEBUG: NOT YET IMPLEMENTED");
}

void PacketSwitch::start() {
	LOG_DEBUG_INTERNAL("Starting packet switch...");

	// Initialize threads
	this->tunnel_tap_read_thread = std::make_unique<std::thread>(&PacketSwitch::tunnel_tap_read_handler, this);
	this->tunnel_socket_read_thread = std::make_unique<std::thread>(&PacketSwitch::tunnel_socket_read_handler, this);
	this->tunnel_tap_write_thread = std::make_unique<std::thread>(&PacketSwitch::tunnel_tap_write_handler, this);
	this->fdb_thread = std::make_unique<std::thread>(&PacketSwitch::fdb_handler, this);

	// Set process nice value
	int niceValue = -10;
	if (setpriority(PRIO_PROCESS, getpid(), niceValue) < 0) {
		throw SuperEthernetTunnelRuntimeException("Could not set process nice value: " + std::string(strerror(errno)));
	}

	// Setup priority for a thread
	int policy = SCHED_RR;
	struct sched_param param;
	param.sched_priority = sched_get_priority_max(SCHED_RR);
	if (pthread_setschedparam(tunnel_tap_read_thread->native_handle(), policy, &param)) {
		LOG_NOTICE("Could not set TAP device thread priority: ", std::strerror(errno));
	}
	if (pthread_setschedparam(tunnel_socket_read_thread->native_handle(), policy, &param)) {
		LOG_NOTICE("Could not set socket read thread priority: ", std::strerror(errno));
	}
	if (pthread_setschedparam(tunnel_tap_write_thread->native_handle(), policy, &param)) {
		LOG_NOTICE("Could not set TAP write thread priority: ", std::strerror(errno));
	}
	// NOTE: We don't need to change the priority of the FDB thread as its not in a critical path

	// Loop with remote nodes and start them
	for (auto &remote_node : this->remote_nodes) {
		remote_node.second->start();
	}

	// Bring the TAP interface online
	this->tap_interface->start();
}

/**
 * @brief Wait for the packet switch to finish.
 */
void PacketSwitch::wait() {
	// Join all threads and wait for them to exit
	this->tunnel_tap_read_thread->join();
	this->tunnel_socket_read_thread->join();
	this->tunnel_tap_write_thread->join();
	this->fdb_thread->join();

	// Loop with each node and wait for it to exit
	for (auto &remote_node : this->remote_nodes) {
		remote_node.second->wait();
	}
}

/**
 * @brief Stop the packet switch.
 */
void PacketSwitch::stop() {
	LOG_NOTICE("Stopping packet switch...");
	this->stop_flag = true;
}

/**
 * @brief Get the max ethernet frame size based on MTU size. This is the raw L2MTU we support.
 *
 * @param mtu Device MTU.
 * @return uint16_t Maximum packet size.
 */

uint16_t get_l2mtu_from_mtu(uint16_t mtu) {
	// Set the initial maximum frame size to the specified MTU
	uint16_t frame_size = mtu;

	// Maximum ethernet frame size is 22 bytes, ethernet header + 802.1ad (8 bytes)
	frame_size += 14 + 8;

	return frame_size;
}

/**
 * @brief Thread responsible for reading from the TAP interface.
 *
 * @param arg Thread data.
 */
void PacketSwitch::tunnel_tap_read_handler() {
	LOG_DEBUG_INTERNAL("TAP READ: Starting TAP read thread");

	// Loop and read data from TAP device
	while (true) {
		// Check for program stop
		if (this->stop_flag) {
			break;
		}

		// Grab a new buffer for our data
		LOG_DEBUG_INTERNAL("AVAIL POOL: Buffer pool count: ", this->available_rx_buffer_pool->getBufferCount(), ", taking one");
		auto buffer = this->available_rx_buffer_pool->pop_wait();

	loop_restart:
		// Read data from TAP interface
		ssize_t bytes_read = read(this->tap_interface->getFD(), buffer->getData(), buffer->getBufferSize());
		if (bytes_read == -1) {
			LOG_ERROR("Got an error read()'ing TAP device: ", strerror(errno));
			exit(EXIT_FAILURE);
		} else if (bytes_read == 0) {
			LOG_ERROR("Got EOF from TAP device: ", strerror(errno));
			exit(EXIT_FAILURE);
		}

		LOG_DEBUG_INTERNAL("TAP READ: Read ", bytes_read, " bytes from TAP");
		buffer->setDataSize(bytes_read);

		// Overlay ethernet header ontop of the packet
		const ethernet_header_t *ethernet_packet = (ethernet_header_t *)buffer->getData();
		// Do a quick sanity check on the source MAC
		if (ethernet_packet->src_mac[0] & 0x01) {
			LOG_ERROR("Packet from ethernet device has a source MAC which is a multicast group address, DROPPING!");
			goto loop_restart;
		}

		// Update FDB entry if we found one, we do this in a separate scope to clear the mutex
		std::shared_ptr<FDBEntry> fdb_entry;
		{
			std::shared_lock<std::shared_mutex> lock(this->fdb_mtx);
			fdb_entry = this->fdb->get((const FDBMACAddress *)&ethernet_packet->src_mac);
			if (fdb_entry) {
				// std::unique_lock<std::shared_mutex> lock(fdb_mtx);
				fdb_entry->updateLastSeen();
			}
		}
		// If we didn't find one, we need to create one, the lock, then add
		if (!fdb_entry) {
			std::unique_lock<std::shared_mutex> lock(this->fdb_mtx);
			fdb->add((const FDBMACAddress *)&ethernet_packet->src_mac, nullptr);
		}

		// Check if the destination MAC is a broadcast address
		if (ethernet_packet->dst_mac[0] & 0x01) {
			// If there is a single node in the remote nodes list, then we can just send it to that node
			if (this->remote_nodes.size() == 1) {
				auto remote_node = this->remote_nodes.begin()->second;
				remote_node->getEncoderPool()->push(std::move(buffer));
			} else {
				// If there are multiple nodes, then we need to send it to all nodes
				for (auto &it : this->remote_nodes) {
					// If this is the last node, send the original buffer
					if (it == *this->remote_nodes.rbegin()) {
						it.second->getEncoderPool()->push(std::move(buffer));
					} else {
						// Otherwise, we need to make a copy of the buffer
						auto buffer_copy = this->available_rx_buffer_pool->pop_wait();
						*buffer_copy = *buffer;
						it.second->getEncoderPool()->push(std::move(buffer_copy));
					}
				}
			}
			// This is not a broadcast, so just send it to the unicast address
		} else {
			std::shared_lock<std::shared_mutex> lock(this->fdb_mtx);
			auto fdb_entry = this->fdb->getRemoteNode((const FDBMACAddress *)ethernet_packet->dst_mac);
			// If we don't have a target or the target is local, then we need ignore this packet and just continue
			if (!fdb_entry) {
				goto loop_restart;
			}
			// We got a target, so send the buffer to the encoder pool
			fdb_entry->getDestination()->getEncoderPool()->push(std::move(buffer));
		}
	}

	LOG_DEBUG_INTERNAL("TAP READ: Exiting TAP read thread");
}

/**
 * @brief Thread responsible for handling statistics.
 *
 * @param encoder Packet encoder.
 * @param tdata Thread data.
 */
/*
void PacketSwitch::tunnel_encoder_stats_handler(PacketEncoder *encoder) {
   // struct ThreadData *tdata = (struct ThreadData *)arg;

   while (true) {
	   // Check for program stop
	   if (this->stop_flag) {
		   break;
	   }

	   // Grab commpression ratio stat from the encoder
	   encoder->getCompressionRatioStat(tdata->statCompressionRatio);

	   LOG_INFO(std::fixed, std::setprecision(2), "STATS: Compression ratio: ", tdata->statCompressionRatio.mean,
				" (min: ", tdata->statCompressionRatio.min, ", max: ", tdata->statCompressionRatio.max, ")");

	   // Sleep for 5 seconds
	   std::this_thread::sleep_for(std::chrono::seconds(5));
   }
}
*/

/**
 * @brief Internal function to compare a packet
 *
 */
struct _comparePacketBuffer {
		bool operator()(const std::unique_ptr<PacketBuffer> &a, const std::unique_ptr<PacketBuffer> &b) const {
			// Use our own comparison operator inside PacketBuffer
			return a->getPacketSequenceKey() < b->getPacketSequenceKey();
		}
};

/**
 * @brief Thread responsible for reading data from the socket.
 *
 * @param arg Thread data.
 */
void PacketSwitch::tunnel_socket_read_handler() {
	ssize_t sockaddr_len = sizeof(sockaddr_storage);
	std::vector<std::unique_ptr<PacketBuffer>> recvmm_buffers(SETH_MAX_RECVMM_MESSAGES);
	struct mmsghdr *msgs;
	struct iovec *iovecs;
	PacketHeader *pkthdr;

	LOG_DEBUG_INTERNAL("Starting socket read thread");

	// START - IO vector buffers

	// Allocate memory for our IO vectors
	msgs = (struct mmsghdr *)malloc(SETH_MAX_RECVMM_MESSAGES * sizeof(struct mmsghdr));
	iovecs = (struct iovec *)malloc(SETH_MAX_RECVMM_MESSAGES * sizeof(struct iovec));

	// Make sure memory allocation succeeded
	if (msgs == NULL || iovecs == NULL) {
		LOG_ERROR("Memory allocation failed for recvmm iovecs: ", strerror(errno));
		// Clean up previously allocated memory
		free(msgs);
		free(iovecs);
		// free(controls);
		throw SuperEthernetTunnelRuntimeException("Memory allocation failed for recvmm iovecs");
	}

	// Set up iovecs and mmsghdrs
	for (size_t i = 0; i < SETH_MAX_RECVMM_MESSAGES; ++i) {
		recvmm_buffers[i] = this->available_tx_buffer_pool->pop_wait();

		// Setup IO vectors
		iovecs[i].iov_base = recvmm_buffers[i]->getData();
		iovecs[i].iov_len = recvmm_buffers[i]->getBufferSize();
		// And the associated msgs
		msgs[i].msg_hdr.msg_iov = &iovecs[i];
		msgs[i].msg_hdr.msg_iovlen = 1;
		msgs[i].msg_hdr.msg_name = recvmm_buffers[i]->getPacketSourceData();
		msgs[i].msg_hdr.msg_namelen = sockaddr_len;
	}
	// END - IO vector buffers

	// Received buffers
	std::map<std::array<uint8_t, 16>, std::deque<std::unique_ptr<PacketBuffer>>> received_buffers;
	while (1) {
		// Check for program stop
		if (this->stop_flag) {
			break;
		}

		// Pull in buffers into iovecs
		int num_received = recvmmsg(this->udp_socket, msgs, SETH_MAX_RECVMM_MESSAGES, MSG_WAITFORONE, NULL);
		if (num_received == -1) {
			throw SuperEthernetTunnelRuntimeException(std::format("recvmmsg failed: {}", strerror(errno)));
		}

		// Loop for each packet received
		for (int i = 0; i < num_received; ++i) {
			// Grab sockaddr storage
			sockaddr_storage *sockaddr = (sockaddr_storage *)recvmm_buffers[i]->getPacketSourceData();
			if (sockaddr->ss_family != AF_INET && sockaddr->ss_family != AF_INET6) {
				LOG_ERROR("Received packet from unknown address family: ", sockaddr->ss_family);
				continue;
			}
			// Overlay sin6_addr ontop of sockaddr to get an IPv6 address for the source
			const sockaddr_in6 *addr6 = reinterpret_cast<const sockaddr_in6 *>(sockaddr);

			char ipv6_str[INET6_ADDRSTRLEN];
			if (inet_ntop(AF_INET6, &(addr6->sin6_addr), ipv6_str, sizeof(ipv6_str)) == NULL) {
				throw SuperEthernetTunnelRuntimeException(std::format("inet_ntop failed: {}", strerror(errno)));
			}
			LOG_DEBUG_INTERNAL("Received ", msgs[i].msg_len, " bytes from ", ipv6_str, ":", ntohs(addr6->sin6_port), " with flags ",
							   msgs[i].msg_hdr.msg_flags);

			// Grab key for the sender
			std::array<uint8_t, 16> node_key = get_key_from_sockaddr(sockaddr);
			// If the node key is not in the remote nodes map, then this host isn't in the access list
			auto it = this->remote_nodes.find(node_key);
			if (it == this->remote_nodes.end()) {
				LOG_ERROR("Received packet from unknown source: ", ipv6_str, ", DROPPING!");
				continue;
			}
			// Save remote node
			auto remote_node = it->second;

			// Grab this IOV's buffer node and update the actual buffer length
			recvmm_buffers[i]->setDataSize(msgs[i].msg_len);

			// Make sure the buffer is long enough for us to overlay the header
			if (recvmm_buffers[i]->getDataSize() < sizeof(PacketHeader) + sizeof(PacketHeaderOption)) {
				LOG_ERROR("Packet too small ", recvmm_buffers[i]->getDataSize(), " < ",
						  sizeof(PacketHeader) + sizeof(PacketHeaderOption), ", DROPPING!!!");
				continue;
			}

			// Overlay packet header and do basic header checks
			pkthdr = (PacketHeader *)recvmm_buffers[i]->getData();
			// Check version is supported
			if (pkthdr->ver > SETH_PACKET_HEADER_VERSION_V1) {
				LOG_ERROR("Packet not supported, version ", static_cast<uint8_t>(pkthdr->ver), " vs. our version ",
						  SETH_PACKET_HEADER_VERSION_V1, ", DROPPING!");
				continue;
			}
			if (pkthdr->reserved != 0) {
				LOG_ERROR("Packet header should not have any reserved its set, it is ", static_cast<uint8_t>(pkthdr->reserved),
						  ", DROPPING!");
				continue;
			}
			// First thing we do is validate the format
			if (pkthdr->format != PacketHeaderFormat::ENCAPSULATED) {
				LOG_ERROR("Packet format not supported, format ", static_cast<uint8_t>(pkthdr->format), ", DROPPING!");
				continue;
			}

			// Next check the channel is set to 0
			if (pkthdr->channel) {
				LOG_ERROR("Packet specifies invalid channel ", pkthdr->channel, ", DROPPING!");
				continue;
			}

			// Add buffer node to the received list
			recvmm_buffers[i]->setPacketSequenceKey(accl::be_to_cpu_32(pkthdr->sequence));

			// Push the buffer into the received buffers list for the node
			received_buffers[node_key].push_back(std::move(recvmm_buffers[i]));

			// Replenish the buffer
			recvmm_buffers[i] = this->available_tx_buffer_pool->pop_wait();
			msgs[i].msg_hdr.msg_iov->iov_base = recvmm_buffers[i]->getData();
			msgs[i].msg_hdr.msg_name = recvmm_buffers[i]->getPacketSourceData();
		}
		// Push all the buffers we got
		for (auto &received_pool : received_buffers) {
			LOG_DEBUG_INTERNAL("Pushing ", received_pool.second.size(), " buffers to decoder pool of node");
			remote_nodes[received_pool.first]->getDecoderPool()->push(received_pool.second);
			received_pool.second.clear();
		}
	}
	// Free the rest of the IOV stuff
	free(msgs);
	free(iovecs);

	LOG_DEBUG_INTERNAL("Exiting socket read thread");
}

/**
 * @brief Thread responsible for writing to the TAP device.
 *
 * @param arg Thread data.
 */
void PacketSwitch::tunnel_tap_write_handler() {
	LOG_DEBUG_INTERNAL("Starting TAP write thread");

	// Loop pulling buffers off the socket write pool
	std::deque<std::unique_ptr<PacketBuffer>> buffers;
	while (true) {
		// Check for program stop
		if (this->stop_flag) {
			break;
		}

		// Wait for buffers
		this->tap_write_pool->wait(buffers);

		// Loop with buffers
#ifdef DEBUG
		size_t i{0};
#endif
		for (auto &buffer : buffers) {
			// Overlay ethernet header ontop of the packet
			const ethernet_header_t *ethernet_packet = (ethernet_header_t *)buffer->getData();

			// Do a quick sanity check on the source MAC
			if (ethernet_packet->src_mac[0] & 0x01) {
				std::string ipv6_str = get_ipstr(&buffer->getPacketSource());
				LOG_ERROR("Packet from ", ipv6_str, " has a source MAC which is a multicast group address, DROPPING!");
				continue;
			}

			// Update FDB entry if we found one, we do this in a separate scope to clear the mutex
			std::shared_ptr<FDBEntry> fdb_entry;
			auto fdb_src_mac = (const FDBMACAddress *)&ethernet_packet->src_mac;
			{
				std::shared_lock<std::shared_mutex> lock(this->fdb_mtx);
				LOG_DEBUG_INTERNAL("Looking for FDB entry for MAC: ", fdb_src_mac->toString());
				fdb_entry = this->fdb->get(fdb_src_mac);
				if (fdb_entry) {
					LOG_DEBUG_INTERNAL("Found FDB entry for MAC: ", fdb_src_mac->toString());
					// std::unique_lock<std::shared_mutex> lock(fdb_mtx);
					fdb_entry->updateLastSeen();
					LOG_DEBUG_INTERNAL("Updated FDB entry for MAC: ", fdb_src_mac->toString());
				}
			}
			// If we didn't find one, we need to create one, the lock, then add
			if (!fdb_entry) {
				LOG_DEBUG_INTERNAL("Adding FDB entry for MAC: ", fdb_src_mac->toString());
				const sockaddr_storage *addr = &buffer->getPacketSource();
				// Grab node that sent this packet
				auto node_key = get_key_from_sockaddr((sockaddr_storage *)addr);
				auto remote_node = this->remote_nodes[node_key];

				std::unique_lock<std::shared_mutex> lock(this->fdb_mtx);
				fdb_entry = fdb->add(fdb_src_mac, remote_node);
				LOG_DEBUG_INTERNAL(std::format("Added FDB entry for MAC: ", fdb_src_mac->toString()));
			}

			// Check if tap device is online, if not skip writing
			if (!this->tap_interface->isOnline()) {
				LOG_WARNING("TAP device is offline, skipping write");
				continue;
			}

			// Write data to TAP interface
			ssize_t bytes_written = write(this->tap_interface->getFD(), buffer->getData(), buffer->getDataSize());
			if (bytes_written == -1) {
				throw SuperEthernetTunnelRuntimeException(std::format("Error writing TAP device: {}", strerror(errno)));
			}
#ifdef DEBUG
			i++;
			LOG_DEBUG_INTERNAL("Wrote ", bytes_written, " bytes to TAP [", i, "/", buffers.size(), "]");
#endif
		}

		// Push buffers into available pool
		this->available_tx_buffer_pool->push(buffers);
		buffers.clear();
	}

	LOG_DEBUG_INTERNAL("Exiting TAP write thread");
}

/**
 * @brief Thread responsible for handling maintenance of the FDB.
 *
 */
void PacketSwitch::fdb_handler() {
	LOG_DEBUG_INTERNAL("Starting FDB maintenance thread");

	// Loop doing maintenance
	while (true) {
		// Check for program stop
		if (this->stop_flag) {
			break;
		}

		// Lock FDB for read and dump it
		{
			std::shared_lock<std::shared_mutex> lock(this->fdb_mtx);
			this->fdb->dumpDebug();
		}
		// Loop with FDB entreis and remove ones that are older than 300s
		{
			std::unique_lock<std::shared_mutex> lock(this->fdb_mtx);
			this->fdb->expireEntries(300);
		}

		sleep(10);
	}

	LOG_DEBUG_INTERNAL("Exiting FDB maintenance thread");
}

/**
 * @brief Create a udp socket.
 *
 * @param tdata Thread data.
 */
void PacketSwitch::_create_udp_socket() {
	// Creating UDP datagram socket
	if ((this->udp_socket = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		throw SuperEthernetTunnelRuntimeException(std::format("ERROR: Socket creation failed: {}", strerror(errno)));
	}

	// Set socket to be dual stack
	int no = 0;
	if (setsockopt(this->udp_socket, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(no)) == -1) {
		int res = errno;
		close(this->udp_socket);
		throw SuperEthernetTunnelRuntimeException(std::format("ERROR: Failed to set IPV6_V6ONLY: {}", strerror(res)));
	}

	// Set the send buffer size (SO_SNDBUF)
	int buffer_size = this->l2mtu * 8192;
	if (setsockopt(this->udp_socket, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size)) < 0) {
		int res = errno;
		close(this->udp_socket);
		throw SuperEthernetTunnelRuntimeException(std::format("ERROR: Failed to set send buffer size: {}", strerror(res)));
	}
	if (setsockopt(this->udp_socket, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) {
		int res = errno;
		close(this->udp_socket);
		throw SuperEthernetTunnelRuntimeException(std::format("ERROR: Failed to set receive buffer size: {}", strerror(res)));
	}

	// Initialize address structure
	struct sockaddr_in6 listen_addr;
	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin6_family = AF_INET6;
	listen_addr.sin6_port = htons(this->port);
	listen_addr.sin6_addr = in6addr_any;

	// Bind UDP socket source address
	if (bind(this->udp_socket, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) == -1) {
		int res = errno;
		close(this->udp_socket);
		throw SuperEthernetTunnelRuntimeException(std::format("ERROR: Failed to bind UDP socket: {}", strerror(res)));
	}
}

/**
 * @brief Destroy a UDP socket.
 *
 * @param tdata Thread data.
 */
void PacketSwitch::_destroy_udp_socket() { close(this->udp_socket); }
