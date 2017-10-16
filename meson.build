project('ucaandor', 'c')

cc = meson.get_compiler('c')

uca_dep = dependency('libuca', version: '>= 2.0')
gio_dep = dependency('gio-2.0', version: '>= 2.0')
atcore_dep = cc.find_library('atcore', dirs: get_option('libatdir'))
atutil_dep = cc.find_library('atutility', dirs: get_option('libatdir'))

plugindir = uca_dep.get_pkgconfig_variable('plugindir')

gnome = import('gnome')

enums = gnome.mkenums('uca-enums',
    sources: ['uca-andor-camera.h'],
    h_template: 'uca-andor-enums.h.template',
    c_template: 'uca-andor-enums.c.template',
)

shared_library('ucaandor',
    sources: ['uca-andor-camera.c', enums[0], enums[1]],
    dependencies: [uca_dep, gio_dep, atcore_dep, atutil_dep],
    install: true,
    install_dir: plugindir,
)
