add_library(dcplayer_dcp_xml_dom STATIC
    internal/xml_dom.cpp
)

target_include_directories(dcplayer_dcp_xml_dom
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_dcp_xml_dom
    PUBLIC
        dcplayer_core
        dcplayer_warnings
)

add_library(dcplayer_dcp_assetmap STATIC
    asset_map.cpp
)

target_include_directories(dcplayer_dcp_assetmap
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_dcp_assetmap
    PUBLIC
        dcplayer_core
        dcplayer_warnings
        dcplayer_dcp_xml_dom
)
