add_executable(secure_channel_contract_test
    secure_channel_contract_test.cpp
)

target_compile_definitions(secure_channel_contract_test
    PRIVATE
        SECURE_CHANNEL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/security/secure_channel"
)

target_link_libraries(secure_channel_contract_test
    PRIVATE
        dcplayer_security_secure_channel
        dcplayer_security_host_spb1_contract
        dcplayer_warnings
)

add_test(NAME secure_channel_contract_unit_test COMMAND secure_channel_contract_test)
set_tests_properties(secure_channel_contract_unit_test PROPERTIES LABELS "unit;security;secure;channel;spb1")

add_executable(spb1_protected_api_test
    spb1_protected_api_test.cpp
)

target_compile_definitions(spb1_protected_api_test
    PRIVATE
        SECURE_CHANNEL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/security/secure_channel"
)

target_link_libraries(spb1_protected_api_test
    PRIVATE
        dcplayer_security_secure_channel
        dcplayer_security_host_spb1_contract
        dcplayer_warnings
)

add_test(NAME spb1_protected_api_unit_test COMMAND spb1_protected_api_test)
set_tests_properties(spb1_protected_api_unit_test PROPERTIES LABELS "unit;security;secure;channel;spb1")
