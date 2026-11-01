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

cmake_minimum_required(VERSION 4.0)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

project(stakeforge)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile;" CACHE STRING "" FORCE)

foreach(LANG C CXX)
    set(CMAKE_${LANG}_FLAGS_PROFILE "${CMAKE_${LANG}_FLAGS_RELEASE}" CACHE STRING "" FORCE)
endforeach()

foreach(LINKER EXE SHARED MODULE STATIC)
    set(CMAKE_${LINKER}_LINKER_FLAGS_PROFILE "${CMAKE_${LINKER}_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)
endforeach()

if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/utils/build_info.py"
        --ensure --cmake-output "${CMAKE_BINARY_DIR}/packaging/build_info.cmake"
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_QUIET
)
include("${CMAKE_BINARY_DIR}/packaging/build_info.cmake")

# ------------- TARGETS -------------

add_subdirectory(stakeforge_core)
add_subdirectory(stakeforge_tools)
add_subdirectory(stakeforge_editor)
include(assets/packaging/packaging.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CustomTargets")
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT stakeforge_editor)
