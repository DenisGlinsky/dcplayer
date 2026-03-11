add_library(dcplayer_video_secure_decode_adapter INTERFACE)

target_include_directories(dcplayer_video_secure_decode_adapter
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_video_secure_decode_adapter
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
