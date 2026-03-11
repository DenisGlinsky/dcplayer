add_library(dcplayer_dcp_ov_vf_resolver INTERFACE)

target_include_directories(dcplayer_dcp_ov_vf_resolver
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_dcp_ov_vf_resolver
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
