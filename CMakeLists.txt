
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

cmake_minimum_required (VERSION 4.0)

project (stakeforge)

set(SFG_CORE_TARGET stakeforge_core)
set(SFG_GAME_TARGET stakeforge_game)
set(SFG_EDITOR_TARGET stakeforge_editor)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;" CACHE STRING "" FORCE)

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

# ------------- COMPILE DEFINITIONS -------------


add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
add_compile_definitions($<$<CONFIG:Debug>:SFG_DEBUG>)
add_compile_definitions($<$<CONFIG:Debug>:SFG_ENABLE_MEMORY_TRACER>)
add_compile_definitions($<$<CONFIG:Debug>:SFG_GFX_SERIALIZE_SHADERS_PDB>)
add_compile_definitions($<$<CONFIG:Debug>:SFG_GFX_USE_DEBUG_LAYERS>)
add_compile_definitions($<$<CONFIG:Debug>:SFG_USE_DEBUG_CONTROLLER>)
add_compile_definitions($<$<CONFIG:Debug>:SFG_DUMP_LOG_TRACE>)
add_compile_definitions($<$<CONFIG:Debug>:USE_PIX>)
add_compile_definitions($<$<CONFIG:Debug>:JPH_DEBUG_RENDERER>)

add_compile_definitions($<$<CONFIG:Release>:_ITERATOR_DEBUG_LEVEL=0>)
add_compile_definitions($<$<CONFIG:Release>:NDEBUG>)
add_compile_definitions($<$<CONFIG:Release>:SFG_DUMP_LOG_TRACE>)
add_compile_definitions($<$<CONFIG:Release>:TRACY_ENABLE>)

add_definitions(-DSFG_CONFIGURATION="$<CONFIGURATION>")
add_definitions(-DSFG_ROOT_DIRECTORY="${CMAKE_SOURCE_DIR}/")

set(CMAKE_C_FLAGS_DEBUGTOOLMODE             "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUGTOOLMODE           "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUGTOOLMODE    "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUGTOOLMODE "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
set(CMAKE_MODULE_LINKER_FLAGS_DEBUGTOOLMODE "${CMAKE_MODULE_LINKER_FLAGS_DEBUG}")
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:ProgramDatabase>")

if (WIN32)
    add_compile_definitions(SFG_PLATFORM_WINDOWS=1)
endif()

if(APPLE)
    add_compile_definitions(SFG_PLATFORM_OSX=1)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_definitions(SFG_COMPILER_CLANG=1)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_definitions(SFG_COMPILER_GNU=1)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_definitions(SFG_COMPILER_MSVC=1)
else()
    add_compile_definitions(SFG_COMPILER_UNKOWN=1)
endif()


# ------------- LANGUAGE -------------

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++23")
set(CMAKE_SUPPRESS_REGENERATION true)

if(MSVC)
    add_definitions("/MP")
    add_compile_options(/GR-)
	add_compile_definitions(_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
endif()

# ------------- SRC -------------

file(GLOB SOURCES 
src/audio/*.cpp
src/common/*.cpp
src/platform/*.cpp
src/math/*.cpp
src/io/*.cpp
src/data/*.cpp
src/memory/*.cpp
src/reflection/*.cpp
src/serialization/*.cpp
src/input/*.cpp
src/project/*.cpp
src/gfx/*.cpp
src/gfx/util/*.cpp
src/gfx/backend/*.cpp
src/gfx/common/*.cpp
src/gfx/world/*.cpp
src/gfx/world/render_pass/*.cpp
src/gfx/event_stream/*.cpp
src/gfx/proxy/*.cpp
src/resources/*.cpp
src/physics/*.cpp
src/gui/*.cpp
src/gui/widgets/*.cpp
src/world/*.cpp
src/world/components/*.cpp
src/world/animation/*.cpp
src/world/particles/*.cpp
src/app/*.cpp
src/sfg_api.cpp
src/sfg_api.cpp
src/vendor/util/*.cpp
src/vendor/miniaudio/*.c
)

file(GLOB HEADERS 
src/audio/*.hpp
src/common/*.hpp
src/platform/*.hpp
src/math/*.hpp
src/io/*.hpp
src/data/*.hpp
src/memory/*.hpp
src/input/*.hpp
src/gfx/*.hpp
src/gfx/util/*.hpp
src/gfx/backend/*.hpp
src/gfx/common/*.hpp
src/gfx/world/*.hpp
src/gfx/world/render_pass/*.hpp
src/gfx/event_stream/*.hpp
src/gfx/proxy/*.hpp
src/gui/*.hpp
src/gui/widgets/*.hpp
src/app/*.hpp
src/project/*.hpp
src/resources/*.hpp
src/reflection/*.hpp
src/serialization/*.hpp
src/physics/*.hpp
src/world/*.hpp
src/world/animation/*.hpp
src/world/particles/*.hpp
src/world/components/*.hpp
src/vendor/stb/*.h
src/vendor/moodycamel/*.h
src/vendor/nhlohmann/*.hpp
src/vendor/util/*.hpp
src/vendor/syoyo/*.h
src/vendor/miniaudio/*.h
src/vendor/um/*.h
src/vendor/um/*.inl
src/sfg_api.hpp
)

file(GLOB GAME_SOURCES

)

file(GLOB GAME_HEADERS

)

file(GLOB EDITOR_SOURCES

)

file(GLOB EDITOR_HEADERS

)

if(WIN32)


file(GLOB PLATFORM_SOURCES 
src/platform/win32/*.cpp
src/gfx/backend/dx12/*.cpp
src/gfx/backend/dx12/sdk/*.cpp
src/gfx/backend/dx12/sdk/*.c
src/gfx/backend/dx12/sdk/Residency/*.cpp
src/gfx/backend/dx12/sdk/Residency/*.c
)

file(GLOB PLATFORM_HEADERS
src/gfx/backend/dx12/*.hpp
src/gfx/backend/dx12/sdk/*.hpp
src/gfx/backend/dx12/sdk/*.h
src/gfx/backend/dx12/sdk/Residency/*.hpp
src/gfx/backend/dx12/sdk/Residency/*.h
)

file(GLOB GAME_PLATFORM_SOURCES
src/game/platform/win32/*.cpp
)

file(GLOB GAME_PLATFORM_HEADERS
src/game/platform/win32/*.hpp
)


file(GLOB EDITOR_PLATFORM_SOURCES
src/editor/platform/win32/*.cpp
)

file(GLOB EDITOR_PLATFORM_HEADERS
src/editor/platform/win32/*.hpp
)

set(ICON ${PROJECT_SOURCE_DIR})

endif()

if(APPLE)

set(ICON ${PROJECT_SOURCE_DIR})

endif()

# ------------- TARGET -------------

if (WIN32)
    add_library(${SFG_CORE_TARGET} STATIC)
    add_executable(${SFG_GAME_TARGET} WIN32 src/game/platform/win32/win32_main.cpp)
    add_executable(${SFG_EDITOR_TARGET} WIN32 src/editor/platform/win32/win32_main.cpp)
else()
    add_library(${SFG_CORE_TARGET} STATIC)
    add_executable(${SFG_GAME_TARGET} src/game/platform/main.cpp)
    add_executable(${SFG_EDITOR_TARGET} src/editor/platform/main.cpp)
endif()

target_sources(${SFG_CORE_TARGET} PRIVATE
    ${SOURCES}
    ${HEADERS}
    ${PLATFORM_HEADERS}
    ${PLATFORM_SOURCES}
    ${ICON}
)

target_compile_definitions(${SFG_CORE_TARGET} PRIVATE
  SFG_MAJOR=1
  SFG_MINOR=0
  SFG_BUILD="${SFG_BUILD}"   # string macro
)


# Shared library (DLL/.dylib/.so)
# add_library(${PROJECT_NAME} SHARED
#     ${SOURCES} ${HEADERS}
#     ${PLATFORM_HEADERS} ${PLATFORM_SOURCES}
#     ${ICON}
# )

include(${CMAKE_SOURCE_DIR}/utils/cmake/executable_settings.cmake)
set_executable_settings(${SFG_GAME_TARGET})
set_executable_settings(${SFG_EDITOR_TARGET})

# ------------- EXPORT HEADER -------------
# include(GenerateExportHeader)
# generate_export_header(${PROJECT_NAME}
#     BASE_NAME SFG
#     EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/sfg_export.h"
# )

target_include_directories(${SFG_CORE_TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/src/)
target_include_directories(${SFG_CORE_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/deps/phmap/include)
target_include_directories(${SFG_CORE_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/deps/tracy-0.13.0/public)


# ------------- DEPS -------------

# JOLT
set(CPP_RTTI_ENABLED OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
set(OVERRIDE_MSVCCRT OFF CACHE BOOL "" FORCE)
set(GENERATE_DEBUG_SYMBOLS ON CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/JoltPhysics-5.4.0/Build)
set_property(TARGET Jolt PROPERTY FOLDER Dependencies)


if (MSVC)
  target_compile_options(Jolt PRIVATE /wd4865 /wd5045)  # ignore C4865 for Jolt
endif()

add_subdirectory(deps/lz4-1.9.4)

add_subdirectory(deps/tracy-0.13.0)
set_property(TARGET TracyClient PROPERTY FOLDER Dependencies)

if(WIN32)

include(deps/dxc/link.cmake)
include(deps/pix/link.cmake)
link_dxc(stakeforge_core)
link_pix(stakeforge_core)

target_link_libraries(${SFG_CORE_TARGET}
            PUBLIC Jolt
            PUBLIC d3d12.lib
            PUBLIC dxgi.lib
            PUBLIC dxguid.lib
        )

add_custom_command(
TARGET ${SFG_GAME_TARGET}
POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/deps/dx12_bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>")

add_custom_command(
TARGET ${SFG_EDITOR_TARGET}
POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/deps/dx12_bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>")

add_custom_command(
TARGET ${SFG_GAME_TARGET}
POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/deps/pix/bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>")

add_custom_command(
TARGET ${SFG_EDITOR_TARGET}
POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/deps/pix/bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>")

endif()

target_link_libraries(${SFG_CORE_TARGET}
PUBLIC lz4
)

target_link_libraries(${SFG_GAME_TARGET} PRIVATE ${SFG_CORE_TARGET})
target_link_libraries(${SFG_EDITOR_TARGET} PRIVATE ${SFG_CORE_TARGET})

target_precompile_headers(${SFG_CORE_TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${PROJECT_SOURCE_DIR}/src/common/pch.hpp>"
)

set_target_properties(lz4 PROPERTIES FOLDER "Dependencies")

include(${CMAKE_SOURCE_DIR}/utils/cmake/common_functions.cmake)
group_sources(HEADERS SOURCES PLATFORM_HEADERS PLATFORM_SOURCES)
group_generated(${SFG_CORE_TARGET})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CustomTargets")
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${SFG_EDITOR_TARGET})

