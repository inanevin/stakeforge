cmake_minimum_required(VERSION 4.0)
project(stakeforge_game LANGUAGES CXX)

find_package(Stakeforge CONFIG REQUIRED)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(stakeforge_game SHARED ${GAME_SOURCES})
target_link_libraries(stakeforge_game PRIVATE Stakeforge::tools)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${GAME_SOURCES})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT stakeforge_game)
