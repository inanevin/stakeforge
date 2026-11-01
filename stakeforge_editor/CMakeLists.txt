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
set_target_properties(${SFG_EDITOR_TARGET} PROPERTIES FOLDER "stakeforge")
sfg_group_sources(PLATFORM_HEADERS PLATFORM_SOURCES)
sfg_group_generated(${SFG_EDITOR_TARGET})
