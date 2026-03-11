add_library(dcplayer_playout_frame_queue INTERFACE)

target_include_directories(dcplayer_playout_frame_queue
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_playout_frame_queue
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
