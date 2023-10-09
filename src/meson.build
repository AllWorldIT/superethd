
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

libsuperethd_sources = ['buffers.c', 'codec.c', 'sockets.c', 'superethd.c', 'tunnel.c', 'tap.c', 'util.c']
libsuperethd = static_library('src', libsuperethd_sources)


#
# Executables
#

executable('superethd-debug', 'main.c', link_with: libsuperethd, dependencies: deps,
    c_args: ['-pg', '-no-pie', '-DDEBUG'],
    link_args: ['-pg', '-no-pie'],
)
executable('superethd', 'main.c', link_with: libsuperethd, dependencies: deps)
