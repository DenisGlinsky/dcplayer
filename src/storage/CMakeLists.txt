add_library(dcplayer_storage INTERFACE)

target_link_libraries(dcplayer_storage
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)

