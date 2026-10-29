#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# This file is a part of: Stakeforge Engine
# https://github.com/inanevin/StakeforgeEngine
# 
# Author: Inan Evin
# http://www.inanevin.com
# 
# Copyright (c) [2025-] [Inan Evin]
# 
# Stakeforge is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# Stakeforge is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.
#
# As an additional permission under section 7 of GPLv3, the copyright
# holders grant the Stakeforge Game Linking Exception, version 1.0,
# in GAME-LINKING-EXCEPTION.md.
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------

set(SFG_EDITOR_TARGET stakeforge_editor)
set(SFG_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../stakeforge_core)
include(${SFG_CORE_DIR}/cmake/compile_definitions.cmake)

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
    ${PLATFORM_HEADERS}
    ${PLATFORM_SOURCES}
    ${SFG_CORE_DIR}/src/sfg/core/memory/memory_operators.cpp
)



if(APPLE)
        set_target_properties(${SFG_EDITOR_TARGET} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if(MSVC)
        set_property(TARGET ${SFG_EDITOR_TARGET} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>/)
endif()
    
target_include_directories(${SFG_EDITOR_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(${SFG_EDITOR_TARGET} PRIVATE stakeforge_tools)

include(${SFG_CORE_DIR}/cmake/common_functions.cmake)
sfg_add_copy_commands(${SFG_EDITOR_TARGET} ${SFG_CORE_DIR})
add_custom_target(stakeforge_editor_shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/common/shaders"
        "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/common/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/common/shaders"
        "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/assets/common/shaders"
    VERBATIM
)
add_dependencies(${SFG_EDITOR_TARGET} stakeforge_editor_shaders)
set_target_properties(stakeforge_editor_shaders PROPERTIES FOLDER "stakeforge")

add_custom_command(
    TARGET ${SFG_EDITOR_TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_scaffold"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_defaults"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_templates"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor/resource_pack"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_defaults"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor_templates"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/common"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${SFG_EDITOR_TARGET}>/editor"
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
sfg_group_sources(PLATFORM_HEADERS PLATFORM_SOURCES)
sfg_group_generated(${SFG_EDITOR_TARGET})
