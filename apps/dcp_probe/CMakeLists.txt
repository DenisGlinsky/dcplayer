add_executable(dcp_probe
    main.cpp
)

target_link_libraries(dcp_probe
    PRIVATE
        dcplayer_core
        dcplayer_dcp
        dcplayer_warnings
)
