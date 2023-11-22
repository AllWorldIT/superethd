<img alt="Super Ethernet Tunnel Logo" src="https://gitlab.conarx.tech/uploads/-/system/project/avatar/92/logo.png" width="256px"/>

A robust and effcient solution for creating virtual ethernet tunnels.

[![License](https://img.shields.io/github/license/grafana/grafana)](LICENSE)
[![pipeline status](https://gitlab.conarx.tech/superethd/superethd/badges/main/pipeline.svg)](https://gitlab.conarx.tech/superethd/superethd/commits/main)
[![coverage report](https://gitlab.conarx.tech/superethd/superethd/badges/main/coverage.svg)](https://gitlab.conarx.tech/superethd/superethd/commits/main)


Super Ethernet Tunnel allows you to create ethernet tunnels while maintaining the original MTU size of the encapsulated packet,
advanced features such as packet stuffing and packet compression help negate the effects of tunneling overheads and fragmentation
while reducing the packet per second processing requirements of intermediate devices.

Features:
- **Raw ethernet frame support:** Super Ethernet Tunnel works with raw ethernet frames and any ethernet protocol, ie. IPv4, IPv6,
ARP, MPLS, VLANs ... etc.
- **Tunnel with virtually any MTU size:** Especially helpful in networks where MSS clamping is not possible or the frames being
transmitted are not TCP or even IP-based.
- **Compression support:** Support is available for both LZ4 and ZSTD packet compression.
- **Packet stuffing:** Packet space is optimized by stuffing multiple payloads per packet and using stream compression to further
improve compression ratios.
- **IPv4 and IPv6 endpoints:** Both IPv4 and IPv6 endpints are supported.


## What Super Ethernet Tunnel is not

Super Ethernet Tunnel is NOT a VPN. It does not implement any encryption, packet authentication or MiTM mitigations. Similar to
other tunneling protocols such as GRE, GENEVE, L2TP ... etc.


## Getting started

Prerequisites:
- Zstd
- LZ4
- Catch2 > 3.4.0 (for tests)


### Building

```bash
meson setup --buildtype release build
ninja -C build
```


### Running

There are a number of options Super Ethernet Tunnel provides to customize its behavior. These can be seen by using
`superethd --help`.

Mandatory arguments:
- **-s &lt;SOURCE&gt;:** Source IPv4 or IPv6 address. We will bind to this address to send traffic outbound.
- **-d &lt;DESTINATION&gt;:** Destination IPv4 or IPv6 address. This is the IP address we will be sending tunnel traffic to.

Optional arguments:
- **-l &lt;LOG_LEVEL&gt;:** Optional log level to specify.
- **-m &lt;MTU&gt;:** Interface MTU. This the MTU of the ethernet tunnel interface. This defaults to 1500.
- **-t &lt;TX_SIZE&gt;:** Maximum transmission unit size. This is the maximum size of the encapsulating packet that will be transmitted.
This defaults to 1500.
- **-p &lt;PORT&gt;:** Port number to use for traffic. Defaults to `58023`.
- **-i &lt;IFNAME&gt;:** Interface name, defaults to `seth0`.
- **-c &lt;COMPRESS_ALGO&gt;:** Compression algorithm to use, defaults to `lz4`.

An example of using this would be...
```bash
superethd -s 192.0.2.1 -d 192.0.2.100
```


## Running tests

Tests need to built separately as they include additional code which is removed out during a release build, here is an example
how to do that.

```bash
meson setup build -Dunit_testing=true
meson test -C build
```


## License

This project is licensed under AGPLv3 or a commercial license at your option. For the full AGPLv3 license text, see the `LICENSE`
file in the repository. For commercial licensing options, please contact Conarx, Ltd at sales@conarx.tech.
