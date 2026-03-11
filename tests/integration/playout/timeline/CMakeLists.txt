add_executable(playback_timeline_integration_test
    playback_timeline_integration_test.cpp
)

target_compile_definitions(playback_timeline_integration_test
    PRIVATE
        OV_VF_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/dcp/ov_vf_resolver"
        SUPPLEMENTAL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/dcp/supplemental"
        TIMELINE_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/playout/timeline"
)

target_link_libraries(playback_timeline_integration_test
    PRIVATE
        dcplayer_dcp_assetmap
        dcplayer_dcp_pkl
        dcplayer_dcp_cpl
        dcplayer_dcp_ov_vf_resolver
        dcplayer_dcp_supplemental
        dcplayer_playout_timeline
        dcplayer_warnings
)

add_test(NAME playback_timeline_integration_test COMMAND playback_timeline_integration_test)
set_tests_properties(playback_timeline_integration_test PROPERTIES LABELS "integration;playout;timeline")
