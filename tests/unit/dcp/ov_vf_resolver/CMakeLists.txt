add_executable(ov_vf_resolver_test
    ov_vf_resolver_test.cpp
)

target_link_libraries(ov_vf_resolver_test
    PRIVATE
        dcplayer_dcp_ov_vf_resolver
        dcplayer_warnings
)

add_test(NAME ov_vf_resolver_unit_test COMMAND ov_vf_resolver_test)
set_tests_properties(ov_vf_resolver_unit_test PROPERTIES LABELS "unit;dcp;ov;vf;resolver")
