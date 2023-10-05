
# Get debug flag so we can enable debugging below
debug = get_option('debug')


conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)

deps = [dependency('threads')]
deps = [dependency('liblz4')]

executable('stap_debug', ['main.c', 'buffers.c', 'packet.c', 'sockets.c', 'tunnel.c', 'tap.c', 'util.c'], dependencies: deps,
    c_args: ['-pg', '-no-pie', '-DDEBUG', '-D_GNU_SOURCE'],
    link_args: ['-pg', '-no-pie'],
)

executable('stap', ['main.c', 'buffers.c', 'sockets.c', 'packet.c', 'tunnel.c', 'tap.c', 'util.c'], dependencies: deps, c_args: ['-D_GNU_SOURCE'])

executable('tst', ['test.c'])
