add_library(dcplayer_playout INTERFACE)

add_subdirectory(timeline)
add_subdirectory(frame_queue)
add_subdirectory(sync)


target_link_libraries(dcplayer_playout
    INTERFACE
        dcplayer_core
        dcplayer_warnings
        dcplayer_playout_timeline
        dcplayer_playout_frame_queue
        dcplayer_playout_sync
)
