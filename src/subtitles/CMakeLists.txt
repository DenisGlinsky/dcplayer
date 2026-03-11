add_library(dcplayer_subtitles INTERFACE)

add_subdirectory(interop)
add_subdirectory(smpte_tt)


target_link_libraries(dcplayer_subtitles
    INTERFACE
        dcplayer_core
        dcplayer_warnings
        dcplayer_subtitles_interop
        dcplayer_subtitles_smpte_tt
)
