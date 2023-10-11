
# Get debug flag so we can enable debugging below
debug = get_option('debug')


conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)


#
# Depdendencies
#

deps = [dependency('threads')]
deps = [dependency('liblz4')]


#
# Static libraries
#

libsuperethd_sources = [
    'buffers.c',
    'checksum.c',
    'codec.c',
    'debug.c',
    'packet.c',
    'sockets.c',
    'superethd.c',
    'tap.c',
    'tunnel.c',
    'util.c',
]
libsuperethd = static_library('superethd', libsuperethd_sources)


#
# Executables
#

executable('superethd-debug', 'main.c', link_with: libsuperethd, dependencies: deps,
    c_args: ['-pg', '-DDEBUG'],
    link_args: ['-pg'],
)
executable('superethd', 'main.c', link_with: libsuperethd, dependencies: deps)
