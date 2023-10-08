
# Get debug flag so we can enable debugging below
debug = get_option('debug')


conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)

deps = [dependency('threads')]
deps = [dependency('liblz4')]

sources = ['main.c', 'buffers.c', 'packet.c', 'sockets.c', 'tunnel.c', 'tap.c', 'util.c']


# FIXME does not work we need this for clang-includes-fixer
find_all_symbols_target = custom_target('find_all_symbols',
  output : 'symbols.yaml',
  command : ['find-all-symbols', sources],
  input : sources,
)


executable('stap_debug', sources, dependencies: deps,
    c_args: ['-pg', '-no-pie', '-DDEBUG', '-D_GNU_SOURCE'],
    link_args: ['-pg', '-no-pie'],
)

executable('stap', [sources], dependencies: deps, c_args: ['-D_GNU_SOURCE'])

executable('tst', ['test.c'])
