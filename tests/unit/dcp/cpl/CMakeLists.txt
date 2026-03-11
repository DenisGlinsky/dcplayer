add_executable(cpl_test
    cpl_test.cpp
)

target_compile_definitions(cpl_test
    PRIVATE
        CPL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/dcp/cpl"
)

target_link_libraries(cpl_test
    PRIVATE
        dcplayer_dcp_cpl
        dcplayer_warnings
)

add_test(NAME cpl_parser_test COMMAND cpl_test)
set_tests_properties(cpl_parser_test PROPERTIES LABELS "unit;dcp;cpl")
