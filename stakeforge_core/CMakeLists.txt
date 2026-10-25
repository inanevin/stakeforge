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
set(TRACY_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(deps/tracy-0.13.1)
target_compile_definitions(TracyClient PUBLIC $<$<CONFIG:Profile>:TRACY_ENABLE> PRIVATE TRACY_EXPORTS)
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
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/audio/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/common/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/platform/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/math/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/io/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/data/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/memory/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/reflection/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/serialization/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/input/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/project/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/util/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/common/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/world/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/world/render_pass/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/event_stream/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/proxy/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/resources/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/animation/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/engine/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/render/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/resources/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/scripting/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/scripting/api/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/world/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/physics/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/project/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gui/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gui/widgets/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/vg/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/layout/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/input/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/paint/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/serialize/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/app/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/sfg_api.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/vendor/util/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/vendor/miniaudio/*.c
)

file(GLOB HEADERS
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/audio/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/common/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/platform/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/math/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/io/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/data/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/memory/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/input/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/util/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/common/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/world/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/world/render_pass/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/event_stream/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/proxy/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gui/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gui/widgets/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/vg/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/layout/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/input/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/paint/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/ui/serialize/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/app/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/project/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/resources/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/animation/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/engine/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/render/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/resources/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/scripting/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/scripting/api/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/world/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/reflection/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/serialization/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/physics/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/runtime/project/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/vendor/stb/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/vendor/moodycamel/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/vendor/nhlohmann/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/vendor/util/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/vendor/syoyo/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/vendor/miniaudio/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/vendor/delfrrr/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/vendor/um/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/vendor/um/*.inl
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/sfg_api.hpp
)

if(WIN32)
    file(GLOB PLATFORM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/platform/win32/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/dx12/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/dx12/sdk/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/dx12/sdk/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/dx12/sdk/Residency/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/dx12/sdk/Residency/*.c
    )

    list(FILTER PLATFORM_SOURCES EXCLUDE REGEX ".*/win32_window\\.cpp$")

    file(GLOB PLATFORM_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/dx12/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/dx12/sdk/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/dx12/sdk/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/dx12/sdk/Residency/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/gfx/backend/dx12/sdk/Residency/*.h
    )
endif()

set(SFG_CORE_TARGET stakeforge_core)

add_library(${SFG_CORE_TARGET} SHARED)

target_compile_definitions(${SFG_CORE_TARGET}
    PUBLIC SFG_CORE_SHARED
    PRIVATE SFG_CORE_BUILDING TRACY_EXPORTS
    INTERFACE TRACY_IMPORTS
)

target_sources(${SFG_CORE_TARGET} PRIVATE
    ${SOURCES}
    ${HEADERS}
    ${PLATFORM_HEADERS}
    ${PLATFORM_SOURCES}
)

target_include_directories(${SFG_CORE_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg
)
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
    add_custom_command(
        TARGET ${SFG_CORE_TARGET}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SFG_DOTNET_NATIVE_HOST_DIR}/nethost.dll"
            "$<TARGET_FILE_DIR:${SFG_CORE_TARGET}>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/deps/dx12_bin"
            "$<TARGET_FILE_DIR:${SFG_CORE_TARGET}>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/deps/pix/bin"
            "$<TARGET_FILE_DIR:${SFG_CORE_TARGET}>"
    )
endif()

target_link_libraries(${SFG_CORE_TARGET} PUBLIC lz4 freetype)
target_link_libraries(${SFG_CORE_TARGET} PUBLIC Tracy::TracyClient)
target_link_libraries(${SFG_CORE_TARGET} PRIVATE ktx)

target_precompile_headers(${SFG_CORE_TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/core/common/pch.hpp>"
)

set_target_properties(${SFG_CORE_TARGET} PROPERTIES FOLDER "stakeforge")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common_functions.cmake)
sfg_group_sources(HEADERS SOURCES PLATFORM_HEADERS PLATFORM_SOURCES)
sfg_group_generated(${SFG_CORE_TARGET})

add_library(stakeforge_memory_overrides OBJECT src/sfg/core/memory/memory_operators.cpp)
target_link_libraries(stakeforge_memory_overrides PUBLIC stakeforge_core)
set_target_properties(stakeforge_memory_overrides PROPERTIES FOLDER "stakeforge")
