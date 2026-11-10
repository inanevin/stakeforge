cmake_minimum_required(VERSION 4.0)
set(SFG_GAME_PROJECT_NAME "stakeforge_game" CACHE STRING "Editor project name")
project("${SFG_GAME_PROJECT_NAME}" LANGUAGES CXX)

find_package(Stakeforge CONFIG REQUIRED)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library("${PROJECT_NAME}" SHARED)

foreach(source IN LISTS GAME_SOURCES)
    file(RELATIVE_PATH relative_source "${CMAKE_CURRENT_SOURCE_DIR}" "${source}")

    if(relative_source MATCHES "^editor/")
        target_sources("${PROJECT_NAME}" PRIVATE "$<$<NOT:$<CONFIG:Packaged>>:${source}>")
    else()
        target_sources("${PROJECT_NAME}" PRIVATE "${source}")
    endif()
endforeach()

target_link_libraries("${PROJECT_NAME}" PRIVATE Stakeforge::core "$<$<NOT:$<CONFIG:Packaged>>:Stakeforge::editor>")
target_compile_definitions("${PROJECT_NAME}" PRIVATE "$<$<NOT:$<CONFIG:Packaged>>:SFG_EDITOR>")
target_include_directories("${PROJECT_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties("${PROJECT_NAME}" PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${GAME_SOURCES})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")

set(_memory_operators "${Stakeforge_DIR}/../include/sfg/core/memory/memory_operators.cpp")
set_source_files_properties("${_memory_operators}" PROPERTIES HEADER_FILE_ONLY FALSE)
target_sources("${PROJECT_NAME}" PRIVATE "${_memory_operators}")
