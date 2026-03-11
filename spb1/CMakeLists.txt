add_library(dcplayer_spb1 INTERFACE)

add_subdirectory(api)
add_subdirectory(simulator)
add_subdirectory(firmware)
add_subdirectory(diag)


target_link_libraries(dcplayer_spb1
    INTERFACE
        dcplayer_core
        dcplayer_warnings
        dcplayer_spb1_api
        dcplayer_spb1_simulator
        dcplayer_spb1_firmware
        dcplayer_spb1_diag
)
