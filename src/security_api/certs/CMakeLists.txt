add_library(dcplayer_security_certs STATIC
    certificate_store.cpp
    trust_chain.cpp
)

target_include_directories(dcplayer_security_certs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_security_certs
    PUBLIC
        dcplayer_core
        dcplayer_warnings
)
