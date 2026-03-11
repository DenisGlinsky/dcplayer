add_executable(playout_service
    main.cpp
)

target_link_libraries(playout_service
    PRIVATE
        dcplayer_core
        dcplayer_playout
        dcplayer_video
        dcplayer_audio
        dcplayer_subtitles
        dcplayer_security_api
        dcplayer_watermark
        dcplayer_warnings
)
