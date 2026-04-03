
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# This file is a part of: Stakeforge Engine
# https://github.com/inanevin/StakeforgeEngine
# 
# Author: Inan Evin
# http://www.inanevin.com
# 
# Copyright (c) [2025-] [Inan Evin]
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------


function(sfg_group_sources)
    set(lists ${ARGV})
    list(LENGTH lists num_lists)
    math(EXPR num_pairs "${num_lists} / 2 - 1")

    foreach(pair RANGE ${num_pairs})
        math(EXPR headers_index "${pair} * 2")
        math(EXPR sources_index "${headers_index} + 1")
        list(GET lists ${headers_index} headers)
        list(GET lists ${sources_index} sources)

        if(MSVC_IDE OR APPLE)
            foreach(source IN LISTS ${headers} ${sources})
                get_filename_component(source_path "${source}" PATH)
                string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" relative_source_path "${source_path}")

                if (MSVC_IDE)
                    string(REPLACE "/" "\\" source_path_ide "${relative_source_path}")
                elseif (APPLE)
                    set(source_path_ide "${relative_source_path}")
                endif()

                source_group("${source_path_ide}" FILES "${source}")
            endforeach()
        endif()
    endforeach()
endfunction()

function(sfg_group_generated target_name)
    source_group("Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target_name}.dir/cmake_pch.cxx)
    source_group("Generated" FILES ${CMAKE_CURRENT_SOURCE_DIR}/_Resources/Game.rc)
    foreach(config_type ${CMAKE_CONFIGURATION_TYPES})
        source_group("Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target_name}.dir/${config_type}/cmake_pch.hxx)
    endforeach()
endfunction()

function (sfg_add_copy_commands target_name sfg_core_dir)
    add_custom_command(
    TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${sfg_core_dir}/deps/dx12_bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>")

    add_custom_command(
    TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${sfg_core_dir}/deps/pix/bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>")

endfunction()