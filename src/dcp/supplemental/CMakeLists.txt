add_library(dcplayer_dcp_supplemental INTERFACE)

target_include_directories(dcplayer_dcp_supplemental
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_dcp_supplemental
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
