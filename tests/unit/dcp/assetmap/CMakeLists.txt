add_executable(asset_map_test
    asset_map_test.cpp
)

target_compile_definitions(asset_map_test
    PRIVATE
        ASSETMAP_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/dcp/assetmap"
)

target_link_libraries(asset_map_test
    PRIVATE
        dcplayer_dcp_assetmap
        dcplayer_warnings
)

add_test(NAME assetmap_parser_test COMMAND asset_map_test)
set_tests_properties(assetmap_parser_test PROPERTIES LABELS "unit;dcp;assetmap")
