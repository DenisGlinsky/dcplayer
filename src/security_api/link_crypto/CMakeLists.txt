add_library(dcplayer_security_link_crypto INTERFACE)

target_include_directories(dcplayer_security_link_crypto
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_security_link_crypto
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
