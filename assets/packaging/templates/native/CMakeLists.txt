cmake_minimum_required(VERSION 4.0)
set(SFG_GAME_PROJECT_NAME "stakeforge_game" CACHE STRING "Editor project name")
project("${SFG_GAME_PROJECT_NAME}" LANGUAGES CXX)

# use Packaged for optimized game DLLs; the editor configuration keeps gameplay unoptimized with symbols.
find_package(Stakeforge CONFIG REQUIRED)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(stakeforge_game SHARED ${GAME_SOURCES})
set_target_properties(stakeforge_game PROPERTIES PROJECT_LABEL "${PROJECT_NAME}")
target_link_libraries(stakeforge_game PRIVATE Stakeforge::tools)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${GAME_SOURCES})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT stakeforge_game)
