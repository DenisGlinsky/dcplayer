add_library(dcplayer_dcp_pkl STATIC
    packing_list.cpp
)

target_include_directories(dcplayer_dcp_pkl
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_dcp_pkl
    PUBLIC
        dcplayer_core
        dcplayer_warnings
        dcplayer_dcp_assetmap
        dcplayer_dcp_xml_dom
)
