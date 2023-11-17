# Build our helper library
subdir('libtests')


# Dependencies

deps = [dependency('catch2-with-main')]


# Build options

inc = ['../src']
libs = [libsuperethd, libsethnetkit, libsuperethdtest]



#
# Buffer tests
#

t_100_buffer = executable('t_100-buffer',
  't_100-buffer.cpp',
  include_directories: inc,
  link_with: libs,
  dependencies: deps,
)
test('Test buffer functionality', t_100_buffer)

t_102_buffer_pool = executable('t_102-buffer-pool',
  't_102-buffer-pool.cpp',
  include_directories: inc,
  link_with: libs,
  dependencies: deps,
)
test('Test buffer pool functionality', t_102_buffer_pool)

t_103_buffer_pool_wait = executable('t_103-buffer-pool-wait',
  't_103-buffer-pool-wait.cpp',
  include_directories: inc,
  link_with: libs,
  dependencies: deps,
)
test('Test buffer pool wait functionality', t_103_buffer_pool_wait)

t_104_buffer_pool_wait_for = executable('t_104-buffer-pool-wait_for',
  't_104-buffer-pool-wait_for.cpp',
  include_directories: inc,
  link_with: libs,
  dependencies: deps,
)
test('Test buffer pool wait_for functionality', t_104_buffer_pool_wait_for)


#
# Sequence generator tests
#

t_120_sequence_generator = executable('t_120-sequence-generator',
  't_120-sequence-generator.cpp',
  include_directories: inc,
  link_with: libs,
  dependencies: deps,
)
test('Test buffer pool functionality', t_120_sequence_generator)



#
# Checksum tests
#

t_200_checksums = executable('t_200-checksums',
  't_200-checksums.cpp',
  include_directories: inc,
  link_with: libs,
  dependencies: deps,
)
test('Test checksums', t_200_checksums)


#
# Packet tests
#

t_400_packet_ethernet = executable('t_400-packet-ethernet',
  't_400-packet-ethernet.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
)
test('Test handling of ethernet packets', t_400_packet_ethernet)


t_410_packet_ethernet_ipv4 = executable('t_410-packet-ethernet-ipv4',
  't_410-packet-ethernet-ipv4.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
)
test('Test handling of ethernet IPv4 packets', t_410_packet_ethernet_ipv4)

t_412_packet_ethernet_ipv6 = executable('t_412-packet-ethernet-ipv6',
  't_412-packet-ethernet-ipv6.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
)
test('Test handling of ethernet IPv6 packets', t_412_packet_ethernet_ipv6)


t_415_packet_ethernet_ipv4_udp = executable('t_415-packet-ethernet-ipv4-udp',
  't_415-packet-ethernet-ipv4-udp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
)
test('Create an ethernet packet with an IPv4 header and UDP header', t_415_packet_ethernet_ipv4_udp)

t_417_packet_ethernet_ipv6_udp = executable('t_417-packet-ethernet-ipv6-udp',
  't_417-packet-ethernet-ipv6-udp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
)
test('Create an ethernet packet with an IPv6 header and UDP header', t_417_packet_ethernet_ipv6_udp)


t_420_packet_ethernet_ipv4_icmp = executable('t_420-packet-ethernet-ipv4-icmp',
  't_420-packet-ethernet-ipv4-icmp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
)
test('Create an ethernet packet with an IPv4 header and ICMP header', t_420_packet_ethernet_ipv4_icmp)

t_422_packet_ethernet_ipv6_icmp = executable('t_422-packet-ethernet-ipv6-icmp',
  't_422-packet-ethernet-ipv6-icmp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
)
test('Create an ethernet packet with an IPv6 header and ICMP header', t_422_packet_ethernet_ipv6_icmp)


t_425_packet_ethernet_ipv4_tcp = executable('t_425-packet-ethernet-ipv4-tcp',
  't_425-packet-ethernet-ipv4-tcp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
)
test('Create an ethernet packet with an IPv4 header and TCP header', t_425_packet_ethernet_ipv4_tcp)

t_427_packet_ethernet_ipv6_tcp = executable('t_427-packet-ethernet-ipv6-tcp',
  't_427-packet-ethernet-ipv6-tcp.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
)
test('Create an ethernet packet with an IPv6 header and TCP header', t_427_packet_ethernet_ipv6_tcp)


#
# Codec tests
#

#stacktrace_deps = [dependency('dl')] + deps

t_998_new_codec = executable('t_998-new-codec',
  't_998-new-codec.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
  #dependencies: stacktrace_deps,
  #cpp_args: stacktrace_args,
)
test('Test codec in development', t_998_new_codec)


t_999_new_codec_benchmark = executable('t_999-new-codec-benchmark',
  't_999-new-codec-benchmark.cpp',
  include_directories: inc,
  link_with:libs,
  dependencies: deps,
)
test('Test codec_benchmark in development', t_999_new_codec_benchmark)
