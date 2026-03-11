add_library(dcplayer_watermark INTERFACE)

add_subdirectory(fm_backend_api)
add_subdirectory(fm_control)
add_subdirectory(detector_harness)


target_link_libraries(dcplayer_watermark
    INTERFACE
        dcplayer_core
        dcplayer_warnings
        dcplayer_watermark_fm_backend_api
        dcplayer_watermark_fm_control
        dcplayer_watermark_detector_harness
)
