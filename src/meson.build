# Meson build system.
# Copyright (C) 2023, AllWorldIT.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

subdir('libsethnetkit')


conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(input: 'config.hpp.in', output: 'config.hpp', configuration: conf_data)


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



