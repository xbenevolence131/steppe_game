cmake_minimum_required(VERSION 3.20)

project(steppe_terrain_generator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
)

add_executable(steppe_engine
    engine/main.cpp
)

target_link_libraries(steppe_engine
    PRIVATE
        steppe_generator
)
