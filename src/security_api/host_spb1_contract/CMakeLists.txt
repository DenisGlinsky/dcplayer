add_library(dcplayer_security_host_spb1_contract INTERFACE)

target_include_directories(dcplayer_security_host_spb1_contract
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_security_host_spb1_contract
    INTERFACE
        dcplayer_core
        dcplayer_warnings
)
