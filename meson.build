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
HEADERS_INSTALL_PATH = join_paths(get_option('prefix'), get_option('includedir'), 'CZ/', meson.project_name())
CONF_FILE_NAME = 'CZ' + meson.project_name() + 'Version.h'

conf = configuration_data()
conf.set('VERSION_MAJOR', VERSION_MAJOR)
conf.set('VERSION_MINOR', VERSION_MINOR)
conf.set('VERSION_PATCH', VERSION_PATCH)
conf_file = configure_file(
    input : 'src/CZ/' + meson.project_name() + '/' + CONF_FILE_NAME +'.in',
    output : CONF_FILE_NAME,
    configuration : conf)

install_data(
    conf_file,
    install_dir: HEADERS_INSTALL_PATH)

# -------------- DEPENDENCIES --------------

cpp                 = meson.get_compiler('cpp')
pkg                 = import('pkgconfig')
cz_core_dep         = dependency('cz-core')
cz_skia_dep         = dependency('cz-skia')
cz_ream_dep         = dependency('cz-ream')
cz_kay_dep          = dependency('cz-kay')

wayland_client_dep  = dependency('wayland-client')
wayland_cursor_dep  = dependency('wayland-cursor')
xkbcommon_dep       = dependency('xkbcommon')
yoga_dep            = dependency('yoga', modules: ['yoga::yogacore'])

deps = [
  cz_core_dep,
  cz_skia_dep,
  cz_ream_dep,
  cz_kay_dep,

  wayland_client_dep,
  wayland_cursor_dep,

  xkbcommon_dep,
  yoga_dep
]

# -------------- SOURCES --------------

header_dirs = [
  ['./src/CZ/Marco', 'CZ/Marco'],
  ['./src/CZ/Marco/Private', 'CZ/Marco/Private'],
  ['./src/CZ/Marco/Roles', 'CZ/Marco/Roles'],
  ['./src/CZ/Marco/Nodes', 'CZ/Marco/Nodes'],
  ['./src/CZ/Marco/Protocols/LvrBackgroundBlur', 'CZ/Marco/Protocols/LvrBackgroundBlur'],
  ['./src/CZ/Marco/Protocols/LvrInvisibleRegion', 'CZ/Marco/Protocols/LvrInvisibleRegion'],
  ['./src/CZ/Marco/Protocols/LvrSvgPath', 'CZ/Marco/Protocols/LvrSvgPath'],
  ['./src/CZ/Marco/Protocols/Wayland', 'CZ/Marco/Protocols/Wayland'],
  ['./src/CZ/Marco/Protocols/WlrLayerShell', 'CZ/Marco/Protocols/WlrLayerShell'],
  ['./src/CZ/Marco/Protocols/XdgShell', 'CZ/Marco/Protocols/XdgShell'],
  ['./src/CZ/Marco/Protocols/XdgDecoration', 'CZ/Marco/Protocols/XdgDecoration'],
  ['./src/CZ/Marco/Protocols/CursorShape', 'CZ/Marco/Protocols/CursorShape'],
  ['./src/CZ/Marco/Protocols/Tablet', 'CZ/Marco/Protocols/Tablet'],
]

foreach header_dir : header_dirs
    header_files = run_command('find', header_dir[0], '-type', 'f', '-name', '*.h', '-maxdepth', '1', check : false).stdout().strip().split('\n')
    install_headers(header_files, install_dir : join_paths(HEADERS_INSTALL_PATH, header_dir[1]))
endforeach

# -------------- LIBRARY --------------

marco = library(
    'cz-marco',
    sources : run_command('find', 'src/CZ', '-type', 'f', '-name', '*[.cpp,.c,.h]', check : false).stdout().strip().split('\n'),
    include_directories : ['src'],
    dependencies : deps,
    soversion: VERSION_MAJOR,
    install : true)

marco_dep = declare_dependency(
    dependencies: deps,
    include_directories : ['src', 'src/CZ/Marco'],
    link_with : marco)

pkg.generate(
    marco,
    name: 'cz-marco',
    description: 'C++ library for building Wayland applications.',
    version: meson.project_version(),
    filebase: 'cz-marco',
    subdirs: 'CZ/',
    requires: [])

# -------------- EXAMPLES --------------

if get_option('build_examples')
    subdir('src/examples/marco-toplevel')
#    subdir('src/examples/marco-toplevel-custom-csd')
    subdir('src/examples/marco-subsurfaces')
    subdir('src/examples/marco-layer-shell')
endif
