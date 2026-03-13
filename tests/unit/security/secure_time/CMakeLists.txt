add_executable(secure_clock_policy_test
    secure_clock_policy_test.cpp
)

target_compile_definitions(secure_clock_policy_test
    PRIVATE
        SECURE_TIME_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/security/secure_time"
)

target_link_libraries(secure_clock_policy_test
    PRIVATE
        dcplayer_security_secure_time
        dcplayer_security_host_spb1_contract
        dcplayer_warnings
)

add_test(NAME secure_clock_policy_unit_test COMMAND secure_clock_policy_test)
set_tests_properties(secure_clock_policy_unit_test PROPERTIES LABELS "unit;security;time;clock;secure_time")

add_executable(secure_time_gate_test
    secure_time_gate_test.cpp
)

target_compile_definitions(secure_time_gate_test
    PRIVATE
        SECURE_TIME_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/security/secure_time"
)

target_link_libraries(secure_time_gate_test
    PRIVATE
        dcplayer_security_secure_time
        dcplayer_security_host_spb1_contract
        dcplayer_warnings
)

add_test(NAME secure_time_gate_unit_test COMMAND secure_time_gate_test)
set_tests_properties(secure_time_gate_unit_test PROPERTIES LABELS "unit;security;time;clock;secure_time")
