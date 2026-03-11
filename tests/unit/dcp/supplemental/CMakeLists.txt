add_executable(supplemental_merge_test
    supplemental_merge_test.cpp
)

target_link_libraries(supplemental_merge_test
    PRIVATE
        dcplayer_dcp_supplemental
        dcplayer_dcp_ov_vf_resolver
        dcplayer_warnings
)

add_test(NAME supplemental_merge_unit_test COMMAND supplemental_merge_test)
set_tests_properties(supplemental_merge_unit_test PROPERTIES LABELS "unit;dcp;supplemental")
