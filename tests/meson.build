# Build our helper library
subdir('libtests')


# Dependencies

deps = [dependency('catch2-with-main')]


# Build options

inc = ['../src']
libs = [libsuperethd, libsethnetkit, libsuperethdtest]


# Tests
test('0100-buffer.cpp',
	executable('t_0100-buffer',
		't_0100-buffer.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0102-buffer-pool.cpp',
	executable('t_0102-buffer-pool',
		't_0102-buffer-pool.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0103-buffer-pool-wait.cpp',
	executable('t_0103-buffer-pool-wait',
		't_0103-buffer-pool-wait.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0104-buffer-pool-wait_for.cpp',
	executable('t_0104-buffer-pool-wait_for',
		't_0104-buffer-pool-wait_for.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0120-sequence-generator.cpp',
	executable('t_0120-sequence-generator',
		't_0120-sequence-generator.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0200-checksums.cpp',
	executable('t_0200-checksums',
		't_0200-checksums.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0400-packet-ethernet.cpp',
	executable('t_0400-packet-ethernet',
		't_0400-packet-ethernet.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0410-packet-ethernet-ipv4.cpp',
	executable('t_0410-packet-ethernet-ipv4',
		't_0410-packet-ethernet-ipv4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0412-packet-ethernet-ipv6.cpp',
	executable('t_0412-packet-ethernet-ipv6',
		't_0412-packet-ethernet-ipv6.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0415-packet-ethernet-ipv4-udp.cpp',
	executable('t_0415-packet-ethernet-ipv4-udp',
		't_0415-packet-ethernet-ipv4-udp.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0417-packet-ethernet-ipv6-udp.cpp',
	executable('t_0417-packet-ethernet-ipv6-udp',
		't_0417-packet-ethernet-ipv6-udp.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0420-packet-ethernet-ipv4-icmp.cpp',
	executable('t_0420-packet-ethernet-ipv4-icmp',
		't_0420-packet-ethernet-ipv4-icmp.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0422-packet-ethernet-ipv6-icmp.cpp',
	executable('t_0422-packet-ethernet-ipv6-icmp',
		't_0422-packet-ethernet-ipv6-icmp.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0425-packet-ethernet-ipv4-tcp.cpp',
	executable('t_0425-packet-ethernet-ipv4-tcp',
		't_0425-packet-ethernet-ipv4-tcp.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0427-packet-ethernet-ipv6-tcp.cpp',
	executable('t_0427-packet-ethernet-ipv6-tcp',
		't_0427-packet-ethernet-ipv6-tcp.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1100-codec-fit.cpp',
	executable('t_1100-codec-fit',
		't_1100-codec-fit.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1110-codec-fit-exact.cpp',
	executable('t_1110-codec-fit-exact',
		't_1110-codec-fit-exact.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1120-codec-fit2.cpp',
	executable('t_1120-codec-fit2',
		't_1120-codec-fit2.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1130-coec-fit2-exact.cpp',
	executable('t_1130-coec-fit2-exact',
		't_1130-coec-fit2-exact.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1200-codec-split.cpp',
	executable('t_1200-codec-split',
		't_1200-codec-split.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1210-codec-split2.cpp',
	executable('t_1210-codec-split2',
		't_1210-codec-split2.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1220-codec-split2-exact.cpp',
	executable('t_1220-codec-split2-exact',
		't_1220-codec-split2-exact.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1600-codec-4-1c1p2c.cpp',
	executable('t_1600-codec-4-1c1p2c',
		't_1600-codec-4-1c1p2c.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1900-codec-seq-wrap.cpp',
	executable('t_1900-codec-seq-wrap',
		't_1900-codec-seq-wrap.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1910-codec-sweep.cpp',
	executable('t_1910-codec-sweep',
		't_1910-codec-sweep.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
