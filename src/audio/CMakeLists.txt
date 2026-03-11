add_library(dcplayer_audio INTERFACE)

add_subdirectory(pcm_engine)
add_subdirectory(channel_router)
add_subdirectory(aes3_adapter)


target_link_libraries(dcplayer_audio
    INTERFACE
        dcplayer_core
        dcplayer_warnings
        dcplayer_audio_pcm_engine
        dcplayer_audio_channel_router
        dcplayer_audio_aes3_adapter
)
