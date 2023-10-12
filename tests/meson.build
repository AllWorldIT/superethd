# Build our helper library
subdir('libtests')


# Dependencies

deps = [dependency('criterion')]


# Build options

inc = ['../src']
libs = [libsuperethd, libsuperethdtest]
c_args += ['-DUNIT_TESTING']
c_args += ['-fprofile-arcs', '-ftest-coverage']

cpp_args = c_args


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


t_415_packet_ethernet_ipv4_udp = executable('t_415-packet-ethernet-ipv4-udp',
  't_415-packet-ethernet-ipv4-udp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Create an ethernet packet with an IPv4 header and UDP header', t_415_packet_ethernet_ipv4_udp)

t_416_packet_ethernet_ipv4_udp_payload = executable('t_416-packet-ethernet-ipv4-udp-payload',
  't_416-packet-ethernet-ipv4-udp-payload.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Create an ethernet packet with an IPv4 header and UDP header with payload', t_416_packet_ethernet_ipv4_udp_payload)


t_420_packet_ethernet_ipv4_icmp = executable('t_420-packet-ethernet-ipv4-icmp',
  't_420-packet-ethernet-ipv4-icmp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  c_args: c_args,
  cpp_args: cpp_args,
)
test('Create an ethernet packet with an IPv4 header and ICMP header', t_420_packet_ethernet_ipv4_icmp)
