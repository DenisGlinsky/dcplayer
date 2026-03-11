add_library(dcplayer_spb1_simulator INTERFACE)

target_include_directories(dcplayer_spb1_simulator
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_spb1_simulator
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
