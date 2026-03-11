add_library(dcplayer_watermark_detector_harness INTERFACE)

target_include_directories(dcplayer_watermark_detector_harness
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_watermark_detector_harness
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
