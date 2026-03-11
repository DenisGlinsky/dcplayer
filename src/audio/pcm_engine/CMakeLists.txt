add_library(dcplayer_audio_pcm_engine INTERFACE)

target_include_directories(dcplayer_audio_pcm_engine
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_audio_pcm_engine
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
