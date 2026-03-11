add_library(dcplayer_subtitles_smpte_tt INTERFACE)

target_include_directories(dcplayer_subtitles_smpte_tt
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_subtitles_smpte_tt
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
