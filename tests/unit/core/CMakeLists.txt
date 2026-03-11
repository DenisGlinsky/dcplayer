add_executable(core_version_test
    core_version_test.cpp
)

target_link_libraries(core_version_test
    PRIVATE
        dcplayer_core
        dcplayer_warnings
)

add_test(NAME core_version_test COMMAND core_version_test)
set_tests_properties(core_version_test PROPERTIES LABELS "unit;core;scaffold")
