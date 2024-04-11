subdir('libaccl')
subdir('libsethnetkit')


conf_data = configuration_data()
conf_data.set('version', meson.project_version())
conf_data.set('sysconfdir', get_option('sysconfdir'))
configure_file(input: 'config.hpp.in', output: 'config.hpp', configuration: conf_data)


#
# Depdendencies
#

cc = meson.get_compiler('cpp')

# if not cc.has_header('netinet/in.h')
#   error('netinet/in.h not found, make sure you have the kernel headers installed')
# endif

deps = [dependency('liblz4')]
deps += [dependency('libzstd')]


#
# Static libraries
#

libsuperethd_sources = [
    'codec.cpp',
    'decoder.cpp',
    'encoder.cpp',
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

executable('superethd', 'main.cpp', link_with: [libsuperethd, libaccl], dependencies: deps, install: true, install_dir: 'bin')


#
# Configuration files
#

install_data('superethd.conf', install_dir: get_option('sysconfdir') / 'superethd')
