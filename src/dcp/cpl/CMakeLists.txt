add_library(dcplayer_dcp_cpl STATIC
    composition_playlist.cpp
)

target_include_directories(dcplayer_dcp_cpl
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_dcp_cpl
    PUBLIC
        dcplayer_core
        dcplayer_warnings
        dcplayer_dcp_xml_dom
)
