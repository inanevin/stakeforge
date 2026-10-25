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

set(SFG_TOOLS_TARGET stakeforge_tools)
set(SFG_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../stakeforge_core)
include(${SFG_CORE_DIR}/cmake/compile_definitions.cmake)

file(GLOB SOURCES
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/assets/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/assets/*/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/commands/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/scripting/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/world/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/panels/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/panels/*/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/widgets/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/widgets/*/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/docking/*.cpp
)

file(GLOB HEADERS
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/assets/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/assets/*/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/commands/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/scripting/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/world/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/panels/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/panels/*/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/widgets/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/widgets/*/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/tools/ui/docking/*.hpp
)


add_library(${SFG_TOOLS_TARGET} SHARED)

target_compile_definitions(${SFG_TOOLS_TARGET}
    PUBLIC SFG_TOOLS_SHARED
    PRIVATE SFG_TOOLS_BUILDING
)

target_sources(${SFG_TOOLS_TARGET} PRIVATE
    ${SOURCES}
    ${HEADERS}
    ${SFG_CORE_DIR}/src/sfg/core/memory/memory_operators.cpp
)
target_include_directories(${SFG_TOOLS_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg
)
target_link_libraries(${SFG_TOOLS_TARGET} PUBLIC stakeforge_core)

set_target_properties(${SFG_TOOLS_TARGET} PROPERTIES FOLDER "stakeforge")
include(${SFG_CORE_DIR}/cmake/common_functions.cmake)
sfg_group_sources(HEADERS SOURCES)
