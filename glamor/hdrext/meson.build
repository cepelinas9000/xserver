srcs_hdrext = [
	'hdrext.c',
	'applyvulkanproperties.c',
	'hdrshaders.c',
	'hdrext_utils.c'
]

headers_hdrext = [
        'hdrproto.h',
]

genres_exe =  executable('hdrext_mkembed','hdrext_mkembed.c')
genres_gen = generator(genres_exe,
  output: '@PLAINNAME@.h',
  arguments: ['@PLAINNAME@', '@INPUT@', '@OUTPUT@'])

embedded_source = genres_gen.process([
        'shaders/bt2020-linear-to-crt-pq-fragment-shader-fp16.glsl',
        'shaders/bt2020-linear-to-crt-pq-vertex-shader-fp16.glsl',
        'shaders/sdr-fragment-shader-fp16.glsl',
        'shaders/sdr-vertex-shader-fp16.glsl',
        'shaders/sdr_fragment_utils.glsl'
        ])


vulkan_dep = dependency('vulkan',version : '>=1.3.283')

libxserver_hdrext = static_library('xserver_hdrext',
        srcs_hdrext,
        embedded_source,
       include_directories: inc,
       dependencies: [common_dep ],
)

build_tests = get_option('tests')
if build_tests
    subdir('tests')
endif