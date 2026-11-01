cmake_minimum_required(VERSION 4.0)
set(SFG_GAME_PROJECT_NAME "stakeforge_game" CACHE STRING "Editor project name")
project("${SFG_GAME_PROJECT_NAME}" LANGUAGES CXX)

find_package(Stakeforge CONFIG REQUIRED)
add_subdirectory(runtime)
add_subdirectory(editor)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")
