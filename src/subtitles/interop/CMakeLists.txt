add_library(dcplayer_subtitles_interop INTERFACE)

target_include_directories(dcplayer_subtitles_interop
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_subtitles_interop
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
