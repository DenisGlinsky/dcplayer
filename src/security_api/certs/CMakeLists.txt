add_library(dcplayer_security_certs INTERFACE)

target_include_directories(dcplayer_security_certs
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_security_certs
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
