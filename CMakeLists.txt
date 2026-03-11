cmake_minimum_required(VERSION 3.25)

project(dcplayer VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DCP_PLAYER_ENABLE_TESTS "Enable unit and integration tests" ON)
option(DCP_PLAYER_ENABLE_SANITIZERS "Enable Address/Undefined sanitizers" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_library(dcplayer_warnings INTERFACE)
target_compile_features(dcplayer_warnings INTERFACE cxx_std_20)

if (MSVC)
    target_compile_options(dcplayer_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(dcplayer_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
    )
endif()

if (DCP_PLAYER_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(dcplayer_warnings INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(dcplayer_warnings INTERFACE -fsanitize=address,undefined)
endif()

add_subdirectory(src/core)
add_subdirectory(src/dcp)
add_subdirectory(src/ingest)
add_subdirectory(src/storage)
add_subdirectory(src/playout)
add_subdirectory(src/video)
add_subdirectory(src/audio)
add_subdirectory(src/subtitles)
add_subdirectory(src/security_api)
add_subdirectory(src/watermark)
add_subdirectory(spb1)

add_subdirectory(apps/dcp_probe)
add_subdirectory(apps/playout_service)
add_subdirectory(apps/sms_ui)
add_subdirectory(apps/tms_web)

if (DCP_PLAYER_ENABLE_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "dcplayer scaffold configured")
