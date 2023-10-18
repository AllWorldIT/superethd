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

project('Super Ethernet Tunnel',
	'cpp',
	version : '0.0.1',
	default_options: ['werror=true', 'c_std=c17', 'cpp_std=c++20'],
)

# Get debug flag so we can enable debugging below
debug = get_option('debug')
if debug
	add_global_arguments('-DDEBUG', language: 'cpp')
endif


unit_testing = get_option('unit_testing')
if unit_testing
	add_global_arguments('-DUNIT_TESTING', language: 'cpp')
	add_global_arguments('-fprofile-arcs', '-ftest-coverage', language: 'cpp')
endif

# Options needed to build everything
add_global_arguments('-D_GNU_SOURCE', language: 'cpp')

# Test suite options
#valgrind_args = ['--error-exitcode=1', '--leak-check=full']
valgrind_args = ['--error-exitcode=1']


subdir('src')
subdir('tests')