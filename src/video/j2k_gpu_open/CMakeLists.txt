add_library(dcplayer_video_j2k_gpu_open INTERFACE)

target_include_directories(dcplayer_video_j2k_gpu_open
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_video_j2k_gpu_open
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
