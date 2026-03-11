add_library(dcplayer_watermark_fm_backend_api INTERFACE)

target_include_directories(dcplayer_watermark_fm_backend_api
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_watermark_fm_backend_api
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
