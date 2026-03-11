add_executable(playback_timeline_test
    playback_timeline_test.cpp
)

target_compile_definitions(playback_timeline_test
    PRIVATE
        TIMELINE_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/playout/timeline"
)

target_link_libraries(playback_timeline_test
    PRIVATE
        dcplayer_dcp_cpl
        dcplayer_dcp_ov_vf_resolver
        dcplayer_playout_timeline
        dcplayer_warnings
)

add_test(NAME playback_timeline_unit_test COMMAND playback_timeline_test)
set_tests_properties(playback_timeline_unit_test PROPERTIES LABELS "unit;playout;timeline")
