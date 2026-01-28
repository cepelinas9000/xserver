srcs_hdrext = [
	'hdrext.c',
	'applyvulkanproperties.c'
]

hdrs_hdrext = [
	'hdrext.h',
	'hdrproto.h',
	'applyvulkanproperties.h'
]

vulkan_dep = dependency('vulkan',version : '>=1.3.283')

libxserver_hdrext = static_library('xserver_hdrext',
	srcs_hdrext,
	include_directories: inc,
	dependencies: common_dep,
)


libxserver += libxserver_hdrext

subdir('tests')
