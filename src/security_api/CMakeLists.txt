add_library(dcplayer_security_api INTERFACE)

add_subdirectory(host_spb1_contract)
add_subdirectory(certs)
add_subdirectory(kdm)
add_subdirectory(link_crypto)
add_subdirectory(log_client)


target_link_libraries(dcplayer_security_api
    INTERFACE
        dcplayer_core
        dcplayer_warnings
        dcplayer_security_host_spb1_contract
        dcplayer_security_certs
        dcplayer_security_kdm
        dcplayer_security_link_crypto
        dcplayer_security_log_client
)
