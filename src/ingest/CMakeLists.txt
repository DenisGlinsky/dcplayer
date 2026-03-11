add_library(dcplayer_ingest INTERFACE)

target_link_libraries(dcplayer_ingest
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)

