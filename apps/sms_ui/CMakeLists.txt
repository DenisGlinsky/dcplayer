add_executable(sms_ui
    main.cpp
)

target_link_libraries(sms_ui
    PRIVATE
        dcplayer_core
        dcplayer_playout
        dcplayer_warnings
)
