add_library(dcplayer_audio_aes3_adapter INTERFACE)

target_include_directories(dcplayer_audio_aes3_adapter
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_audio_aes3_adapter
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
