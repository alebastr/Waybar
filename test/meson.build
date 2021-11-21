test_inc = include_directories('../include')
test_dep = [
    catch2,
    fmt,
    gtkmm,
    jsoncpp,
    spdlog,
]

config_test = executable(
    'config_test',
    'config.cpp',
    '../src/config.cpp',
    dependencies: test_dep,
    include_directories: test_inc,
)

safesignal_test = executable(
    'safesignal_test',
    'SafeSignal.cpp',
    dependencies: test_dep,
    include_directories: test_inc,
)

json_test = executable(
    'json_test',
    'json.cpp',
    dependencies: test_dep,
    include_directories: test_inc,
)

test(
    'Configuration test',
    config_test,
    workdir: meson.source_root(),
)

test(
    'SafeSignal test',
    safesignal_test,
    workdir: meson.source_root(),
)

test(
    'json deserialization test',
    json_test,
    workdir: meson.source_root(),
)
