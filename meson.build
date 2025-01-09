project(
    'Marco',
    'c','cpp',
    version : '0.1.0',
    meson_version: '>= 0.62.0',
    default_options: [
        'warning_level=2',
        'buildtype=release',
        'cpp_std=c++20'])

VERSION_MAJOR = meson.project_version().split('.')[0]
VERSION_MINOR = meson.project_version().split('.')[1]
VERSION_PATCH = meson.project_version().split('.')[2]
VERSION_BUILD = run_command('cat', './BUILD', check : false).stdout()

HEADERS_INSTALL_PATH = join_paths(get_option('prefix'), get_option('includedir'), 'Marco')

# -------------- DEPENDENCIES --------------

cpp                 = meson.get_compiler('cpp')
pkg                 = import('pkgconfig')
wayland_client_dep  = dependency('wayland-client')
wayland_egl_dep     = dependency('wayland-egl')
kay_dep             = dependency('Kay')

# -------------- HEADERS --------------

header_dirs = [
    ['./src/Marco', ''],
    ['./src/Marco/roles', 'roles'],
    ['./src/Marco/protocols', 'protocols']
]

foreach header_dir : header_dirs
    header_files = run_command('find', header_dir[0], '-type', 'f', '-name', '*.h', '-maxdepth', '1', check : false).stdout().strip().split('\n')
    install_headers(header_files, install_dir : join_paths(HEADERS_INSTALL_PATH, header_dir[1]))
endforeach

# -------------- LIBRARY --------------

marco = library(
    'Marco',
    sources : run_command('find', './src/Marco', '-type', 'f', '-name', '*[.cpp,.c]', check : false).stdout().strip().split('\n'),
    include_directories : ['./src'],
    dependencies : [
        wayland_client_dep,
        wayland_egl_dep,
        kay_dep
    ],
    soversion: VERSION_MAJOR,
    install : true)

marco_dep = declare_dependency(
    dependencies: [kay_dep],
    include_directories : ['./src'],
    link_with : marco)

pkg.generate(
    marco,
    name: 'Marco',
    description: 'C++ library for building Wayland applications.',
    version: meson.project_version(),
    filebase: 'Marco')

# -------------- EXAMPLES --------------

if get_option('build_examples')
    subdir('src/examples/marco-basic')
endif
