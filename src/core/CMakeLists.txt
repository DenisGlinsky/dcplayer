add_library(dcplayer_core STATIC
    version.cpp
)

target_include_directories(dcplayer_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dcplayer_core
    PUBLIC
        dcplayer_warnings
)
