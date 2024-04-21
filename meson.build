# Meson build system.
# Copyright (C) 2023, AllWorldIT.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

project('superethd',
	'cpp',
	version : '0.2.1',
	default_options: ['werror=true', 'c_std=gnu17', 'cpp_std=gnu++20', 'b_pie=true'],
)

add_global_arguments('-pedantic-errors', language: 'cpp')

# Get debug flag so we can enable debugging below
debug = get_option('debug')
if debug
	add_global_arguments('-DDEBUG', language: 'cpp')
	add_global_arguments('-fno-omit-frame-pointer', language: 'cpp')
	add_global_arguments('-g3', language: 'cpp')
	add_global_arguments('-DBOOST_STACKTRACE_USE_ADDR2LINE', language: 'cpp')
endif


with_tests = get_option('with_tests')
if with_tests
	add_global_arguments('-DUNIT_TESTING', language: 'cpp')
	add_global_arguments('-fprofile-arcs', '-ftest-coverage', language: 'cpp')
endif

# Options needed to build everything
add_global_arguments('-D_GNU_SOURCE', language: 'cpp')

# Test suite options
valgrind_args = ['--error-exitcode=1', '--leak-check=full', '--show-leak-kinds=all']


subdir('src')
subdir('systemd')
if with_tests
	subdir('tests')
endif