add_library(dcplayer_security_kdm INTERFACE)

target_include_directories(dcplayer_security_kdm
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_security_kdm
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
