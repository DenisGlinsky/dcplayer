add_library(dcplayer_dcp INTERFACE)

add_subdirectory(assetmap)
add_subdirectory(pkl)
add_subdirectory(cpl)
add_subdirectory(ov_vf_resolver)
add_subdirectory(supplemental)


target_link_libraries(dcplayer_dcp
    INTERFACE
        dcplayer_core
        dcplayer_warnings
        dcplayer_dcp_assetmap
        dcplayer_dcp_pkl
        dcplayer_dcp_cpl
        dcplayer_dcp_ov_vf_resolver
        dcplayer_dcp_supplemental
)
