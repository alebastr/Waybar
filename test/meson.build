catch2 = dependency('catch2',
    required: get_option('tests'),
    fallback: ['catch2', 'catch2_dep'],
)

if catch2.found()
    test_files = files(
        '../src/bar_config.cpp',
        'SafeSignal.cpp',
        'bar_config.cpp',
        'main.cpp',
    )

    test_exe = executable('catch2.bin',
        test_files,
        dependencies: [
            catch2,
            fmt,
            gtkmm,
            jsoncpp,
            spdlog
        ],
        include_directories: include_dirs,
    )

    test('catch2 test', test_exe,
      workdir: meson.current_source_dir())
endif
