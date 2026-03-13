add_executable(audit_event_test
    audit_event_test.cpp
)

target_compile_definitions(audit_event_test
    PRIVATE
        AUDIT_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/security/audit"
)

target_link_libraries(audit_event_test
    PRIVATE
        dcplayer_security_audit
        dcplayer_warnings
)

add_test(NAME audit_event_unit_test COMMAND audit_event_test)
set_tests_properties(audit_event_unit_test PROPERTIES LABELS "unit;security;audit;event")

add_executable(audit_export_tamper_test
    audit_export_tamper_test.cpp
)

target_compile_definitions(audit_export_tamper_test
    PRIVATE
        AUDIT_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/security/audit"
)

target_link_libraries(audit_export_tamper_test
    PRIVATE
        dcplayer_security_audit
        dcplayer_warnings
)

add_test(NAME audit_export_tamper_unit_test COMMAND audit_export_tamper_test)
set_tests_properties(audit_export_tamper_unit_test PROPERTIES LABELS "unit;security;audit;tamper;export")
