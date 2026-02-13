srcs_hdrext = [
	'hdrext.c',
	'applyvulkanproperties.c',
	'hdrshaders.c',
	'hdrext_utils.c'
]

headers_hdrext = [
        'hdrproto.h',
]

xxd_exe = find_program('xxd')
xxd_gen = generator(xxd_exe,
  output: '@PLAINNAME@.h',
  arguments: ['-i', '-C', '-n', '@PLAINNAME@', '@INPUT@', '@OUTPUT@'])

embedded_source = xxd_gen.process([
        'shaders/bt2020-linear-to-crt-pq-fragment-shader-fp16.glsl',
        'shaders/bt2020-linear-to-crt-pq-vertex-shader-fp16.glsl',
        'shaders/sdr-fragment-shader-fp16.glsl',
        'shaders/sdr-vertex-shader-fp16.glsl',
        ])


vulkan_dep = dependency('vulkan',version : '>=1.3.283')

libxserver_hdrext = static_library('xserver_hdrext',
        srcs_hdrext,
        embedded_source,
       include_directories: inc,
       dependencies: [common_dep ],
)


subdir('tests')
