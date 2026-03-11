add_library(dcplayer_playout_sync INTERFACE)

target_include_directories(dcplayer_playout_sync
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_playout_sync
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
