add_executable(ov_vf_integration_test
    ov_vf_integration_test.cpp
)

target_compile_definitions(ov_vf_integration_test
    PRIVATE
        OV_VF_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/dcp/ov_vf_resolver"
)

target_link_libraries(ov_vf_integration_test
    PRIVATE
        dcplayer_dcp_assetmap
        dcplayer_dcp_pkl
        dcplayer_dcp_cpl
        dcplayer_dcp_ov_vf_resolver
        dcplayer_warnings
)

add_test(NAME ov_vf_resolver_integration_test COMMAND ov_vf_integration_test)
set_tests_properties(ov_vf_resolver_integration_test PROPERTIES LABELS "integration;dcp;ov;vf;resolver")
