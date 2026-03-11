add_executable(supplemental_merge_integration_test
    supplemental_merge_integration_test.cpp
)

target_compile_definitions(supplemental_merge_integration_test
    PRIVATE
        OV_VF_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/dcp/ov_vf_resolver"
        SUPPLEMENTAL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/dcp/supplemental"
)

target_link_libraries(supplemental_merge_integration_test
    PRIVATE
        dcplayer_dcp_assetmap
        dcplayer_dcp_pkl
        dcplayer_dcp_cpl
        dcplayer_dcp_ov_vf_resolver
        dcplayer_dcp_supplemental
        dcplayer_warnings
)

add_test(NAME supplemental_merge_integration_test COMMAND supplemental_merge_integration_test)
set_tests_properties(supplemental_merge_integration_test PROPERTIES LABELS "integration;dcp;supplemental")
