add_library(dcplayer_audio_channel_router INTERFACE)

target_include_directories(dcplayer_audio_channel_router
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_audio_channel_router
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
