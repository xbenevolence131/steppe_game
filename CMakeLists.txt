cmake_minimum_required(VERSION 3.20)

project(steppe_terrain_generator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    cpp_httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.18.7.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(cpp_httplib)

add_library(steppe_generator STATIC
    engine/steppe_generator.cpp
    engine/steppe_generator.h
)

target_include_directories(steppe_generator
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/engine
)

add_library(steppe_game STATIC
    game/game_mode.h
    game/game_state.cpp
    game/game_state.h
)

target_include_directories(steppe_game
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/game
)

target_link_libraries(steppe_game
    PUBLIC
        steppe_generator
        nlohmann_json::nlohmann_json
)

add_executable(steppe_engine
    engine/main.cpp
)

target_link_libraries(steppe_engine
    PRIVATE
        steppe_game
)

add_executable(steppe_daemon
    engine/steppe_daemon.cpp
)

target_link_libraries(steppe_daemon
    PRIVATE
        steppe_game
        httplib::httplib
        nlohmann_json::nlohmann_json
)
