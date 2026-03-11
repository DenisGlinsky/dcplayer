add_library(dcplayer_dcp_supplemental STATIC
    supplemental_merge.cpp
)

target_include_directories(dcplayer_dcp_supplemental
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_dcp_supplemental
    PUBLIC
        dcplayer_core
        dcplayer_dcp_assetmap
        dcplayer_dcp_pkl
        dcplayer_dcp_cpl
        dcplayer_dcp_ov_vf_resolver
        dcplayer_warnings
)
