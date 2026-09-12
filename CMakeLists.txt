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

find_package(Git QUIET)
set(SFG_BUILD "unknown")
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE SFG_BUILD
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

# ------------- TARGETS -------------

# build managed script hosting dll
if(WIN32)
    find_program(SFG_DOTNET_EXECUTABLE NAMES dotnet REQUIRED)

    set(SFG_DOTNET_HOST_PACK_ROOT "$ENV{ProgramFiles}/dotnet/packs/Microsoft.NETCore.App.Host.win-x64")
    file(GLOB SFG_DOTNET_HOST_PACK_VERSIONS
        RELATIVE "${SFG_DOTNET_HOST_PACK_ROOT}"
        "${SFG_DOTNET_HOST_PACK_ROOT}/10.*")
    list(SORT SFG_DOTNET_HOST_PACK_VERSIONS COMPARE NATURAL ORDER DESCENDING)

    if(NOT SFG_DOTNET_HOST_PACK_VERSIONS)
        message(FATAL_ERROR ".NET 10 x64 native hosting pack was not found at ${SFG_DOTNET_HOST_PACK_ROOT}.")
    endif()

    list(GET SFG_DOTNET_HOST_PACK_VERSIONS 0 SFG_DOTNET_HOST_PACK_VERSION)

    set(SFG_DOTNET_NATIVE_HOST_DIR
        "${SFG_DOTNET_HOST_PACK_ROOT}/${SFG_DOTNET_HOST_PACK_VERSION}/runtimes/win-x64/native")
    set(SFG_MANAGED_SCRIPT_HOST_PROJECT
        "${CMAKE_SOURCE_DIR}/stakeforge_managed/Stakeforge.ScriptHost/Stakeforge.ScriptHost.csproj")
    set(SFG_MANAGED_SCRIPT_HOST_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/managed/$<CONFIGURATION>")

    add_custom_target(stakeforge_managed_script_host ALL
        COMMAND "${SFG_DOTNET_EXECUTABLE}" build
            "${SFG_MANAGED_SCRIPT_HOST_PROJECT}"
            --nologo
            --configuration "$<CONFIGURATION>"
            --output "${SFG_MANAGED_SCRIPT_HOST_OUTPUT_DIRECTORY}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        VERBATIM)
endif()

add_subdirectory(stakeforge_core)
add_subdirectory(stakeforge_editor)
add_subdirectory(stakeforge_game)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CustomTargets")
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT stakeforge_editor)

if(TARGET stakeforge_managed_script_host)
    set_target_properties(stakeforge_managed_script_host PROPERTIES FOLDER "stakeforge/managed")
endif()
