# Meson build system.
# Copyright (C) 2023, AllWorldIT.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.


# Build our helper library
subdir('libtests')


# Dependencies

deps = [dependency('catch2-with-main')]


# Build options

inc = ['../src']
libs = [libsuperethd, libsethnetkit, libsuperethdtest]



#
# Checksum function tests
#

t_200_checksums = executable('t_200-checksums',
  't_200-checksums.cpp',
  include_directories: inc,
  link_with: libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Checksum functions', t_200_checksums)


#
# Packet tests
#

t_400_packet_ethernet = executable('t_400-packet-ethernet',
  't_400-packet-ethernet.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Test handling of ethernet packets', t_400_packet_ethernet)


t_410_packet_ethernet_ipv4 = executable('t_410-packet-ethernet-ipv4',
  't_410-packet-ethernet-ipv4.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Test handling of ethernet IPv4 packets', t_410_packet_ethernet_ipv4)

t_412_packet_ethernet_ipv6 = executable('t_412-packet-ethernet-ipv6',
  't_412-packet-ethernet-ipv6.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Test handling of ethernet IPv6 packets', t_412_packet_ethernet_ipv6)


t_415_packet_ethernet_ipv4_udp = executable('t_415-packet-ethernet-ipv4-udp',
  't_415-packet-ethernet-ipv4-udp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Create an ethernet packet with an IPv4 header and UDP header', t_415_packet_ethernet_ipv4_udp)

t_417_packet_ethernet_ipv6_udp = executable('t_417-packet-ethernet-ipv6-udp',
  't_417-packet-ethernet-ipv6-udp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Create an ethernet packet with an IPv6 header and UDP header', t_417_packet_ethernet_ipv6_udp)


t_420_packet_ethernet_ipv4_icmp = executable('t_420-packet-ethernet-ipv4-icmp',
  't_420-packet-ethernet-ipv4-icmp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Create an ethernet packet with an IPv4 header and ICMP header', t_420_packet_ethernet_ipv4_icmp)

t_422_packet_ethernet_ipv6_icmp = executable('t_422-packet-ethernet-ipv6-icmp',
  't_422-packet-ethernet-ipv6-icmp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Create an ethernet packet with an IPv6 header and ICMP header', t_422_packet_ethernet_ipv6_icmp)


t_425_packet_ethernet_ipv4_tcp = executable('t_425-packet-ethernet-ipv4-tcp',
  't_425-packet-ethernet-ipv4-tcp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Create an ethernet packet with an IPv4 header and TCP header', t_425_packet_ethernet_ipv4_tcp)

t_427_packet_ethernet_ipv6_tcp = executable('t_427-packet-ethernet-ipv6-tcp',
  't_427-packet-ethernet-ipv6-tcp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Create an ethernet packet with an IPv6 header and TCP header', t_427_packet_ethernet_ipv6_tcp)
