add_library(dcplayer_watermark_fm_control INTERFACE)

target_include_directories(dcplayer_watermark_fm_control
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_watermark_fm_control
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
