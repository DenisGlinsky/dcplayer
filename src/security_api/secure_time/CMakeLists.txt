add_library(dcplayer_security_secure_time STATIC
    secure_time.cpp
)

target_include_directories(dcplayer_security_secure_time
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_security_secure_time
    PUBLIC
        dcplayer_core
        dcplayer_security_secure_channel
        dcplayer_warnings
)
