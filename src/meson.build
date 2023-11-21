subdir('libaccl')
subdir('libsethnetkit')


conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(input: 'config.hpp.in', output: 'config.hpp', configuration: conf_data)


#
# Depdendencies
#

deps = [dependency('blosc2')]
deps += [dependency('liblz4')]


#
# Static libraries
#

libsuperethd_sources = [
    'Decoder.cpp',
    'Encoder.cpp',
    'debug.cpp',
    'sockets.cpp',
    'superethd.cpp',
    'tap.cpp',
    'tunnel.cpp',
    'util.cpp',
]
libsuperethd = static_library('superethd', libsuperethd_sources)


#
# Executables
#

executable('superethd', 'main.cpp', link_with: [libsuperethd, libaccl], dependencies: deps)
