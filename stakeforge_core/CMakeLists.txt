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


# ------------- COMPILE DEFINITIONS -------------

include(cmake/compile_definitions.cmake)

# ------------- DEPS -------------

add_subdirectory(deps/JoltPhysics-5.4.0/Build)
set_target_properties(Jolt PROPERTIES FOLDER "stakeforge/dependencies")

add_subdirectory(deps/lz4-1.9.4)
set_target_properties(lz4 PROPERTIES FOLDER "stakeforge/dependencies")

add_subdirectory(deps/tracy-0.13.0)
set_target_properties(TracyClient PROPERTIES FOLDER "stakeforge/dependencies")

if(MSVC)
    target_compile_options(Jolt PRIVATE /wd4865 /wd5045)
endif()

# ------------- SOURCES -------------

file(GLOB SOURCES
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/audio/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/common/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/platform/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/math/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/io/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/data/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/memory/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/reflection/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/serialization/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/input/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/project/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/util/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/common/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/world/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/world/render_pass/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/event_stream/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/proxy/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/resources/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/physics/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gui/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gui/widgets/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/app/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/sfg_api.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/util/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/miniaudio/*.c
)

file(GLOB HEADERS
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/audio/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/common/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/platform/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/math/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/io/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/data/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/memory/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/input/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/util/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/common/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/world/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/world/render_pass/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/event_stream/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/proxy/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gui/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gui/widgets/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/app/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/project/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/resources/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/reflection/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/serialization/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/physics/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/stb/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/moodycamel/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/nhlohmann/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/util/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/syoyo/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/miniaudio/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/um/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/um/*.inl
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/sfg_api.hpp
)

if(WIN32)
    file(GLOB PLATFORM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/platform/win32/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/dx12/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/dx12/sdk/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/dx12/sdk/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/dx12/sdk/Residency/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/dx12/sdk/Residency/*.c
    )

    list(FILTER PLATFORM_SOURCES EXCLUDE REGEX ".*/win32_window\\.cpp$")

    file(GLOB PLATFORM_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/dx12/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/dx12/sdk/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/dx12/sdk/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/dx12/sdk/Residency/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gfx/backend/dx12/sdk/Residency/*.h
    )
endif()

set(SFG_CORE_TARGET stakeforge_core)

add_library(${SFG_CORE_TARGET} STATIC)

target_sources(${SFG_CORE_TARGET} PRIVATE
    ${SOURCES}
    ${HEADERS}
    ${PLATFORM_HEADERS}
    ${PLATFORM_SOURCES}
)

target_include_directories(${SFG_CORE_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg)
target_include_directories(${SFG_CORE_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/deps/phmap/include)
target_include_directories(${SFG_CORE_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/tracy-0.13.0/public)

if(WIN32)
    include(${CMAKE_CURRENT_SOURCE_DIR}/deps/dxc/link.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/deps/pix/link.cmake)
    link_dxc(${SFG_CORE_TARGET})
    link_pix(${SFG_CORE_TARGET})

    target_link_libraries(${SFG_CORE_TARGET}
        PUBLIC Jolt
        PUBLIC d3d12.lib
        PUBLIC dxgi.lib
        PUBLIC dxguid.lib
        PUBLIC Dwmapi.lib
        PUBLIC Shcore.lib
        PUBLIC Winmm.lib
    )
endif()

target_link_libraries(${SFG_CORE_TARGET} PUBLIC lz4)

target_precompile_headers(${SFG_CORE_TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/common/pch.hpp>"
)

set_target_properties(${SFG_CORE_TARGET} PROPERTIES FOLDER "stakeforge")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common_functions.cmake)
sfg_group_sources(HEADERS SOURCES PLATFORM_HEADERS PLATFORM_SOURCES)
sfg_group_generated(${SFG_CORE_TARGET})
