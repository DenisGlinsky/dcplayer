add_library(dcplayer_security_audit STATIC
    audit.cpp
)

target_include_directories(dcplayer_security_audit
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_security_audit
    PUBLIC
        dcplayer_core
        dcplayer_security_secure_channel
        dcplayer_warnings
)
