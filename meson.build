

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