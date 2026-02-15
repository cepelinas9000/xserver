# tests with simple dependencies
hdr_simple_dep_tests = [
    'test_applyvulkanproperties.c',
    'test_hdrext.c'
]

criterion_dep = dependency('criterion')


aditional_srcs = ['../../../os/string.c']
foreach simple_test : hdr_simple_dep_tests
    e = executable(simple_test + '.bin', [simple_test] + aditional_srcs,
                        dependencies: [ criterion_dep, common_dep, epoxy_dep ],
                        include_directories: inc,
                        link_with : [libxserver_hdrext]
                        )
    test('hdr_simple/' + simple_test,e, args: '--tap')

endforeach

