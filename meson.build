

project('Super Ethernet Tunnel',
	'cpp',
	version : '0.0.1',
	default_options: ['werror=true', 'c_std=c17', 'cpp_std=c++20'],
)

# Options needed to build everything
add_global_arguments('-D_GNU_SOURCE', language: 'cpp')
# Test suite options
valgrind_args = ['--error-exitcode=1', '--leak-check=full']


subdir('src')
subdir('tests')