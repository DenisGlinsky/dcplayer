add_executable(tms_web
    main.cpp
)

target_link_libraries(tms_web
    PRIVATE
        dcplayer_core
        dcplayer_security_api
        dcplayer_warnings
)
