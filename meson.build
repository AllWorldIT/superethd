

project('Super Ethernet Tunnel', 'c',
	version : '0.0.1',
	default_options: ['werror=true', 'c_std=c17'],
)

# Options needed to build everything
add_global_arguments('-D_GNU_SOURCE', language: 'c')

subdir('src')
subdir('tests')