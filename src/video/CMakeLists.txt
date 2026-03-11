add_library(dcplayer_video INTERFACE)

add_subdirectory(j2k_cpu)
add_subdirectory(j2k_gpu_open)
add_subdirectory(secure_decode_adapter)


target_link_libraries(dcplayer_video
    INTERFACE
        dcplayer_core
        dcplayer_warnings
        dcplayer_video_j2k_cpu
        dcplayer_video_j2k_gpu_open
        dcplayer_video_secure_decode_adapter
)
