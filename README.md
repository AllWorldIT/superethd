<img alt="Super Ethernet Tunnel Logo" src="https://gitlab.conarx.tech/uploads/-/system/project/avatar/92/logo.png" width="256px"/>

A robust and efficient solution for creating virtual ethernet tunnels.

[![License](https://img.shields.io/github/license/grafana/grafana)](LICENSE)
[![pipeline status](https://gitlab.conarx.tech/superethd/superethd/badges/main/pipeline.svg)](https://gitlab.conarx.tech/superethd/superethd/commits/main)
[![coverage report](https://gitlab.conarx.tech/superethd/superethd/badges/main/coverage.svg)](https://gitlab.conarx.tech/superethd/superethd/commits/main)


Super Ethernet Tunnel is one of the only tunneling technologies that allows you to create ethernet tunnel while maintaining the
original MTU size of the encapsulated packet. Advanced features such as packet stuffing and packet compression help negate the
effects of tunneling overheads and packet fragmenting while reducing the packet per second processing requirements of intermediate
devices.

Features:
- **Raw ethernet frame support:** Super Ethernet Tunnel works with raw ethernet frames and any ethernet protocol, ie. IPv4, IPv6,
ARP, MPLS, VLANs (802.1q, 802.1ad) ... etc.
- **Tunnel with virtually any modern MTU size:** Especially helpful in networks where MSS clamping is not possible either or the
frames being transmitted are not TCP or even IP-based. The inner layer 2 frames are supported up to sizes of 9218 bytes.
-- **Full mesh support:** Automatic full mesh networking is supported by simply pointing Super Ethernet Tunnel to all nodes you
wish to be added to the same layer 2 network. Full mesh networking additionally uses split horizon switching which prevents loops
of traffic between nodes.
- **Compression support:** Support is available for both LZ4 and ZSTD packet compression. Parallel compression of stream data is
implemented when dealing with mulitple nodes.
- **Packet stuffing:** Packet space is optimized by stuffing multiple payloads per packet and using stream compression to further
improve compression ratios.
- **IPv4 and IPv6 endpoints:** Both IPv4 and IPv6 endpoints are supported.
- **Multithreaded:** Super Ethernet Tunnel is multithreaded and takes advantage of multiple cores scaling linearly depending on the
number of nodes being connected.

Requirements:
- Modern Linux system. (GCC13.2+)
- Fixed IP's on both sides of the tunnel.

Wishlist:
- **RST documentation:** With additional features we'll need a proper site with RST based documentation.
- **Detailed statistics:** Would be nice to see RTT on each node, compression ratio to/from each node, dropped packet count, pps,
bps to/from each node. Export via JSON configurable (eg. 30s).
- **CLI tool:** Display things like the statistics and FDB table. Export via JSON.
- **Node authentication support:** It would be nice if nodes could authenticate themselves to the remote node so static IP's both
sides are not required, or in other words dynamic endpoint registration.
- **Add TCP support:** With TCP support we could probably implement a fully stream-based compression approach allowing the
compression algorithm to adapt to the data being compressed and achieve much higher compression ratios.
- **TAP:** Investigate use of other IOCTL options, especially multiqueue, our next performance target is 10Gbit/s.
- **Kernel driver:** Implementing a new kernel driver that supports modern IOVEC access mechanisms would greatly improve performance
over the TAP inteface approach.
- **Builtin web interface:** A builtin web interface would be amazing.


If you find this project useful and want to see new features implemented, please kindly consider supporting me on Patreon
(https://www.patreon.com/opensourcecoder). Initial design and PoC of this project took me around 1,250 hours.


## What Super Ethernet Tunnel is not

Super Ethernet Tunnel is NOT a VPN. It does not currently implement encryption, packet authentication or MiTM mitigations. It is
however similar to other tunneling protocols such as GRE, GENEVE, L2TP, VXLAN ... etc.


## Getting started

Prerequisites:
- GCC 13.2.1+
- Boost
- Zstd
- LZ4
- Catch2 > 3.4.0 (for tests)
- gcovr (for coverage report)


### Building

Build Super Ethernet Tunnel with the release build type...

```bash
meson setup --buildtype release build
ninja -C build
```


### Installing

To install Super Ethernet Tunnel you can use the following...

```bash
ninja -C build install
```


### Running

There are a number of options Super Ethernet Tunnel provides to customize its behavior. These can be seen by using
`superethd --help`.

Mandatory arguments:
- **-s &lt;SOURCE&gt;:** Source IPv4 or IPv6 address. We will bind to this address to send traffic outbound.
- **-d &lt;DESTINATION&gt;:** Destination IPv4 or IPv6 address. This is the IP address we will be sending tunnel traffic to. This
can be a comma, semicolon or space separated list.

Optional arguments:
- **-l &lt;LOG_LEVEL&gt;:** Optional log level to specify.
- **-m &lt;MTU&gt;:** Interface MTU. This the MTU of the ethernet tunnel interface. This defaults to 1500.
- **-t &lt;TX_SIZE&gt;:** Maximum transmission unit size. This is the maximum size of the encapsulating packet that will be transmitted.
This must be the SAME on ALL nodes. This defaults to 1500.
- **-p &lt;PORT&gt;:** Port number to use for traffic. Defaults to `58023`.
- **-i &lt;IFNAME&gt;:** Interface name, defaults to `seth0`.
- **-c &lt;COMPRESS_ALGO&gt;:** Compression algorithm to use, defaults to `lz4`.

To establish a full mesh L2 network between two hosts one could use the following... (assuming the other two hosts are .100 and .200)
```bash
superethd -d 192.0.2.100 -d 192.0.2.200
```

One can also use a configuration file, typically located in `/etc/superethd/superethd.conf`...
```ini
# Log level: debug, info, notice, warning, error
#loglevel=info

# MTU size of the ethernet interface
#mtu=1500

# Maximum TX size that the path between both source and destination can accomodate
#txsize=1500

# Source IPv4/IPv6 of the host on which Super Ethernet Tunnel is running
source=192.0.2.1

# Destination IPv4/IPv6 of the host which we're tunneling traffic to
destination=192.168.2.100

# UDP port to use for our tunnel
#port=58023

# Ethernet interface name
#interface=seth0

# Compression algorithm to use: none, lz4, zstd
#compression=lz4
```

If one is using the systemd service, additional configuration files can be created in `/etc/superethd` with the name
in the format of `superethd-<IFACE>.conf`, the `superethd@` serivce can then be used matching the interface name. eg.
`superethd@seth1`.


For multiple tunnels on a single host, specify a differnt port number per tunnel.


## Running tests

Tests need to built separately as they include additional code which is removed out during a release build, here is an example
how to do that.

```bash
meson setup build -Dunit_testing=true
meson test -C build
```


## Documentation

  * [Contributing](https://gitlab.oscdev.io/oscdev/contributing/-/blob/master/README.md)


## Support

  * [Issue Tracker](https://gitlab.conarx.tech/superethd/superethd/-/issues)
  * [Discord](https://discord.gg/j5CngkSfYs)
  * [Support my work on Patreon](https://www.patreon.com/opensourcecoder)


## License

This project is licensed under AGPLv3 or a commercial license at your option. For the full AGPLv3 license text, see the `LICENSE`
file in the repository. For commercial licensing options, please contact Conarx, Ltd on sales@conarx.tech.
