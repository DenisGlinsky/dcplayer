add_library(dcplayer_dcp_cpl INTERFACE)

target_include_directories(dcplayer_dcp_cpl
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_dcp_cpl
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
