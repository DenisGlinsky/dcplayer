add_executable(security_pki_test
    pki_test.cpp
)

target_compile_definitions(security_pki_test
    PRIVATE
        PKI_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/security/certs"
)

target_link_libraries(security_pki_test
    PRIVATE
        dcplayer_security_certs
        dcplayer_warnings
)

add_test(NAME security_pki_unit_test COMMAND security_pki_test)
set_tests_properties(security_pki_unit_test PROPERTIES LABELS "unit;security;pki;cert;trust")
