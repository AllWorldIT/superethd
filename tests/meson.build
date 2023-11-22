# Build our helper library
subdir('libtests')


# Dependencies

deps = [dependency('catch2-with-main')]
deps += [dependency('liblz4')]
deps += [dependency('libzstd')]


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
test('0130-compressor-lz4.cpp',
	executable('t_0130-compressor-lz4',
		't_0130-compressor-lz4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('0140_compressor-zstd.cpp',
	executable('t_0140_compressor-zstd',
		't_0140_compressor-zstd.cpp',
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
test('1105-codec-fit-lz4.cpp',
	executable('t_1105-codec-fit-lz4',
		't_1105-codec-fit-lz4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1106-codec-fit-zstd.cpp',
	executable('t_1106-codec-fit-zstd',
		't_1106-codec-fit-zstd.cpp',
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
test('1115-codec-fit-exact-lz4.cpp',
	executable('t_1115-codec-fit-exact-lz4',
		't_1115-codec-fit-exact-lz4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1116-codec-fit-exact-zstd.cpp',
	executable('t_1116-codec-fit-exact-zstd',
		't_1116-codec-fit-exact-zstd.cpp',
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
test('1125-codec-fit2-lz4.cpp',
	executable('t_1125-codec-fit2-lz4',
		't_1125-codec-fit2-lz4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1126-codec-fit2-zstd.cpp',
	executable('t_1126-codec-fit2-zstd',
		't_1126-codec-fit2-zstd.cpp',
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
test('1135-coec-fit2-exact-lz4.cpp',
	executable('t_1135-coec-fit2-exact-lz4',
		't_1135-coec-fit2-exact-lz4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1136-coec-fit2-exact-zstd.cpp',
	executable('t_1136-coec-fit2-exact-zstd',
		't_1136-coec-fit2-exact-zstd.cpp',
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
test('1205-codec-split-lz4.cpp',
	executable('t_1205-codec-split-lz4',
		't_1205-codec-split-lz4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1206-codec-split-zstd.cpp',
	executable('t_1206-codec-split-zstd',
		't_1206-codec-split-zstd.cpp',
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
test('1215-codec-split2-lz4.cpp',
	executable('t_1215-codec-split2-lz4',
		't_1215-codec-split2-lz4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1216-codec-split2-zstd.cpp',
	executable('t_1216-codec-split2-zstd',
		't_1216-codec-split2-zstd.cpp',
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
test('1225-codec-split2-exact-lz4.cpp',
	executable('t_1225-codec-split2-exact-lz4',
		't_1225-codec-split2-exact-lz4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1226-codec-split2-exact-zstd.cpp',
	executable('t_1226-codec-split2-exact-zstd',
		't_1226-codec-split2-exact-zstd.cpp',
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
test('1605-codec-4-1c1p2c-lz4.cpp',
	executable('t_1605-codec-4-1c1p2c-lz4',
		't_1605-codec-4-1c1p2c-lz4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1606-codec-4-1c1p2c-zstd.cpp',
	executable('t_1606-codec-4-1c1p2c-zstd',
		't_1606-codec-4-1c1p2c-zstd.cpp',
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
test('1905-codec-seq-wrap-lz4.cpp',
	executable('t_1905-codec-seq-wrap-lz4',
		't_1905-codec-seq-wrap-lz4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1905-codec-seq-wrap-zstd.cpp',
	executable('t_1905-codec-seq-wrap-zstd',
		't_1905-codec-seq-wrap-zstd.cpp',
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
test('1915-codec-sweep-lz4.cpp',
	executable('t_1915-codec-sweep-lz4',
		't_1915-codec-sweep-lz4.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
test('1916-codec-sweep-zstd.cpp',
	executable('t_1916-codec-sweep-zstd',
		't_1916-codec-sweep-zstd.cpp',
		include_directories: inc,
		link_with: libs,
		dependencies: deps,
	)
)
