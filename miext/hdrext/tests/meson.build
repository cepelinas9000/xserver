# tests with simple dependencies
hdr_simple_dep_tests = [
    'test_applyvulkanproperties.c',
]


criterion_dep = dependency('criterion')


foreach simple_test : hdr_simple_dep_tests
    e = executable(simple_test + '.bin', simple_test,
                        dependencies: [ criterion_dep, common_dep ],
                        include_directories: inc,
                        link_with: [libxserver_hdrext]
                        )
    test('hdr_simple/' + simple_test,e, args: '--tap')

endforeach

