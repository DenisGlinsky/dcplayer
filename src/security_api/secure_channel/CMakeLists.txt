add_library(dcplayer_security_secure_channel STATIC
    secure_channel.cpp
)

target_include_directories(dcplayer_security_secure_channel
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_security_secure_channel
    PUBLIC
        dcplayer_core
        dcplayer_warnings
)
