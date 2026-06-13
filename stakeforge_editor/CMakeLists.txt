#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# This file is a part of: Stakeforge Engine
# https://github.com/inanevin/StakeforgeEngine
# 
# Author: Inan Evin
# http://www.inanevin.com
# 
# Copyright (c) [2025-] [Inan Evin]
# 
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 
#    1. Redistributions of source code must retain the above copyright notice, this
#       list of conditions and the following disclaimer.
# 
#    2. Redistributions in binary form must reproduce the above copyright notice,
#       this list of conditions and the following disclaimer in the documentation
#       and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------

set(SFG_EDITOR_TARGET stakeforge_editor)
set(SFG_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../stakeforge_core)
include(${SFG_CORE_DIR}/cmake/compile_definitions.cmake)

file(GLOB SOURCES
${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/assets/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/ui/panels/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/ui/widgets/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/ui/docking/*.cpp
)

file(GLOB HEADERS
${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/assets/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/ui/panels/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/ui/widgets/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/ui/docking/*.hpp
)


if(WIN32)
    file(GLOB PLATFORM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win32/*.cpp
    )

    file(GLOB PLATFORM_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win32/*.hpp
    )

    add_executable(${SFG_EDITOR_TARGET} WIN32)
else()
    add_executable(${SFG_EDITOR_TARGET})
endif()

target_sources(${SFG_EDITOR_TARGET} PRIVATE
    ${SOURCES}
    ${HEADERS}
    ${PLATFORM_HEADERS}
    ${PLATFORM_SOURCES}
)



if(APPLE)
        set_target_properties(${SFG_EDITOR_TARGET} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if(MSVC)
        set_property(TARGET ${SFG_EDITOR_TARGET} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>/)
endif()
    
target_include_directories(${SFG_EDITOR_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${SFG_EDITOR_TARGET} PRIVATE stakeforge_core)

include(${SFG_CORE_DIR}/cmake/common_functions.cmake)
sfg_add_copy_commands(${SFG_EDITOR_TARGET} ${SFG_CORE_DIR})
add_custom_command(
    TARGET ${SFG_EDITOR_TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_scaffold"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_defaults"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_templates"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor/resource_pack"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_defaults"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_templates"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/editor/defaults/shaders"
        "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_defaults/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/editor/defaults/materials"
        "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_defaults/materials"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/editor/defaults/samplers"
        "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_defaults/samplers"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/editor/defaults/textures"
        "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_defaults/textures"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/editor/defaults/sky"
        "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_defaults/sky"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/editor/templates/shaders"
        "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_templates/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/editor/templates/materials"
        "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_templates/materials"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/editor/templates/samplers"
        "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_templates/samplers"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/editor/resource_pack"
        "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor/resource_pack"
)
set_target_properties(${SFG_EDITOR_TARGET} PROPERTIES FOLDER "stakeforge")
sfg_group_sources(HEADERS SOURCES PLATFORM_HEADERS PLATFORM_SOURCES)
sfg_group_generated(${SFG_EDITOR_TARGET})
