subdir('libsethnetkit')


conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)


#
# Depdendencies
#

deps = [dependency('threads')]
deps = [dependency('liblz4')]

c_args = []
c_args_debug = c_args
c_args_debug += ['-pg', '-DDEBUG']

cpp_args = c_args
cpp_args_debug = c_args_debug

link_args = []
link_args_debug = ['-pg']


#
# Static libraries
#

libsuperethd_sources = [
    'buffers.cpp',
    'codec.cpp',
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

executable('superethd-debug', 'main.cpp', link_with: libsuperethd, dependencies: deps,
    c_args: c_args_debug,
    cpp_args: cpp_args_debug,
    link_args: link_args_debug,
)
executable('superethd', 'main.cpp', link_with: libsuperethd, dependencies: deps)



