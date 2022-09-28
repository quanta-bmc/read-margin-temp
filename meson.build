project(
    'read-margin-temp',
    'cpp',
    version: '1.0',
    default_options: [
        'cpp_std=c++20',
    ],
    meson_version: '>=0.57.0',
)

executable(
    'read-margin-temp',
    [
        'main.cpp',
        'sensor/sensor.cpp',
        'util/util.cpp',
        'json/buildjson.cpp',
    ],
    dependencies: [
        dependency('sdbusplus'),
        dependency('phosphor-dbus-interfaces'),
        dependency('sdeventplus'),
    ],
    install: true,
    install_dir: get_option('bindir')
)

#install_data(sources : 'config-margin.json', install_dir : '/usr/share/read-margin-temp')

conf_data = configuration_data()

configure_file(output : 'config.h',
               configuration : conf_data)
