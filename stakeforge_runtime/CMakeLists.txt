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

set(SFG_RUNTIME_TARGET stakeforge_runtime)
set(SFG_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../stakeforge_core)
include(${SFG_CORE_DIR}/cmake/compile_definitions.cmake)

file(GLOB SOURCES
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/engine/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/render/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/resources/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/world/*.cpp
)

file(GLOB HEADERS
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/engine/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/render/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/resources/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/sfg/world/*.hpp
)

add_library(${SFG_RUNTIME_TARGET} STATIC)

target_sources(${SFG_RUNTIME_TARGET} PRIVATE
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(${SFG_RUNTIME_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/sfg)
target_link_libraries(${SFG_RUNTIME_TARGET} PUBLIC stakeforge_core)

set_target_properties(${SFG_RUNTIME_TARGET} PROPERTIES FOLDER "stakeforge")
include(${SFG_CORE_DIR}/cmake/common_functions.cmake)
sfg_group_sources(HEADERS SOURCES)
sfg_group_generated(${SFG_RUNTIME_TARGET})
