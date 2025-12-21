amodesetting_srcs = [
    'dri2.c',
    'driver.c',
    'drmmode_display.c',
    'dumb_bo.c',
    'pageflip.c',
    'present.c',
    'vblank.c',
]

if epoxy_dep.found()
    egl_dep = epoxy_dep
else
    egl_dep = disabler()
endif

shared_module(
    'amodesetting_drv',
    amodesetting_srcs,
    name_prefix: '',

    include_directories: [inc, xorg_inc],
    c_args: xorg_c_args,
    dependencies: [
        common_dep,
        egl_dep,
        udev_dep,
        libdrm_dep,
        gbm_dep,
    ],

    install: true,
    install_dir: join_paths(module_abi_dir, 'drivers'),
)

# Test that we don't have any unresolved symbols from our module to Xorg.
xorg_build_root = join_paths(meson.project_build_root(), 'hw', 'xfree86')
symbol_test_args = []
symbol_test_args += join_paths(xorg_build_root, 'libxorgserver.so')
symbol_test_args += join_paths(xorg_build_root, 'dixmods', 'libshadow.so')
if build_glx
  symbol_test_args += join_paths(xorg_build_root, 'dixmods', 'libglx.so')
endif
if gbm_dep.found()
    symbol_test_args += join_paths(xorg_build_root, 'glamor_egl', 'libglamoregl.so')
endif
symbol_test_args += join_paths(xorg_build_root, 'drivers', 'video', 'amodesetting', 'amodesetting_drv.so')

install_data('10-amodesetting.conf', install_dir: join_paths(get_option('datadir'), 'X11/xorg.conf.d') )