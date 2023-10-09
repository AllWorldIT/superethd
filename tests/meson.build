

#
# Tests
#

test_executable = executable('test-packet-encoding-decoding',
  'packet-encoding-decoding.c',
  include_directories: '../src',
  link_with: libsuperethd,
  dependencies: deps,
)
test('Packet encoding and decoding', test_executable)

