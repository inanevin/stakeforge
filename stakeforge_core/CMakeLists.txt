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

set(FT_DISABLE_ZLIB TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI TRUE CACHE BOOL "" FORCE)
add_subdirectory(deps/freetype-2.14.3)
set_target_properties(freetype PROPERTIES FOLDER "stakeforge/dependencies")

set(TRACY_ENABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/tracy-0.13.1)
target_compile_definitions(TracyClient PUBLIC $<$<CONFIG:Profile>:TRACY_ENABLE>)
set_target_properties(TracyClient PROPERTIES FOLDER "stakeforge/dependencies")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_DOC OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_JNI OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_PY OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS_CTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_LOADTEST_APPS OFF CACHE STRING "" FORCE)
set(KTX_FEATURE_KTX1 ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_KTX2 ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_ETC_UNPACK OFF CACHE BOOL "" FORCE)
set(KTX_GIT_VERSION_FULL "v4.4.2")
add_subdirectory(deps/KTX-Software-4.4.2)
set_target_properties(ktx PROPERTIES FOLDER "stakeforge/dependencies")
set_target_properties(ktx_read PROPERTIES FOLDER "stakeforge/dependencies")
set_target_properties(astcenc-avx2-static PROPERTIES FOLDER "stakeforge/dependencies")
set_target_properties(ktx_version PROPERTIES FOLDER "stakeforge/dependencies")
set_target_properties(obj_basisu_cbind PROPERTIES FOLDER "stakeforge/dependencies")
set_target_properties(objUtil PROPERTIES FOLDER "stakeforge/dependencies")

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
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/job/*.cpp
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
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/animation/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/engine/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/render/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/resources/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/scripting/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/scripting/api/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/world/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/physics/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/project/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gui/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/gui/widgets/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/vg/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/layout/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/input/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/paint/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/serialize/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/app/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/sfg_api.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/util/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/miniaudio/*.c
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/algorithm/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/core/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/cuda/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/cuda/algorithm/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/utility/*.cpp
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
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/job/*.hpp
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
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/vg/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/layout/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/input/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/paint/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/ui/serialize/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/app/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/project/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/resources/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/animation/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/engine/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/render/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/resources/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/scripting/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/scripting/api/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/world/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/reflection/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/serialization/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/physics/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/runtime/project/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/stb/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/moodycamel/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/nhlohmann/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/util/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/syoyo/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/miniaudio/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/um/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/um/*.inl
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/algorithm/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/core/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/cuda/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/cuda/algorithm/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/vendor/taskflow/utility/*.hpp
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

target_include_directories(${SFG_CORE_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${SFG_CORE_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/deps/phmap/include)

if(WIN32)
    include(${CMAKE_CURRENT_SOURCE_DIR}/deps/dxc/link.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/deps/pix/link.cmake)
    link_dxc(${SFG_CORE_TARGET})
    link_pix(${SFG_CORE_TARGET})

    target_link_libraries(${SFG_CORE_TARGET}
        PUBLIC Jolt
        PUBLIC ${SFG_DOTNET_NATIVE_HOST_DIR}/nethost.lib
        PUBLIC d3d12.lib
        PUBLIC dxgi.lib
        PUBLIC dxguid.lib
        PUBLIC Dwmapi.lib
        PUBLIC Shcore.lib
        PUBLIC Winmm.lib
    )
endif()

if(WIN32)
    target_include_directories(${SFG_CORE_TARGET} PRIVATE ${SFG_DOTNET_NATIVE_HOST_DIR})
endif()

target_link_libraries(${SFG_CORE_TARGET} PUBLIC lz4 freetype)
target_link_libraries(${SFG_CORE_TARGET} PUBLIC Tracy::TracyClient)
target_link_libraries(${SFG_CORE_TARGET} PRIVATE ktx)

target_precompile_headers(${SFG_CORE_TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/common/pch.hpp>"
)

set_target_properties(${SFG_CORE_TARGET} PROPERTIES FOLDER "stakeforge")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common_functions.cmake)
sfg_group_sources(HEADERS SOURCES PLATFORM_HEADERS PLATFORM_SOURCES)
sfg_group_generated(${SFG_CORE_TARGET})
