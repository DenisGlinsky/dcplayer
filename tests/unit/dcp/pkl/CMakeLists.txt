add_executable(packing_list_test
    packing_list_test.cpp
)

target_compile_definitions(packing_list_test
    PRIVATE
        ASSETMAP_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/dcp/assetmap"
        PKL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/dcp/pkl"
)

target_link_libraries(packing_list_test
    PRIVATE
        dcplayer_dcp_assetmap
        dcplayer_dcp_pkl
        dcplayer_warnings
)

add_test(NAME pkl_parser_test COMMAND packing_list_test)
set_tests_properties(pkl_parser_test PROPERTIES LABELS "unit;dcp;pkl")
