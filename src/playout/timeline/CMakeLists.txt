add_library(dcplayer_playout_timeline STATIC
    playback_timeline.cpp
)

target_include_directories(dcplayer_playout_timeline
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_playout_timeline
    PUBLIC
        dcplayer_core
        dcplayer_dcp_cpl
        dcplayer_dcp_ov_vf_resolver
        dcplayer_warnings
)
