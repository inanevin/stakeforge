
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
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${sfg_core_dir}/../assets"
        "$<TARGET_FILE_DIR:${target_name}>/assets")

    # copy nethost dll to target,
    # copy engine managed dll, deps json and runtime config to target
    if(WIN32)
        add_dependencies(${target_name} stakeforge_managed_script_host)

        add_custom_command(
        TARGET ${target_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target_name}>/managed"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SFG_DOTNET_NATIVE_HOST_DIR}/nethost.dll"
            "$<TARGET_FILE_DIR:${target_name}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SFG_MANAGED_SCRIPT_HOST_OUTPUT_DIRECTORY}/Stakeforge.ScriptHost.dll"
            "$<TARGET_FILE_DIR:${target_name}>/managed"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SFG_MANAGED_SCRIPT_HOST_OUTPUT_DIRECTORY}/Stakeforge.ScriptHost.deps.json"
            "$<TARGET_FILE_DIR:${target_name}>/managed"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SFG_MANAGED_SCRIPT_HOST_OUTPUT_DIRECTORY}/Stakeforge.ScriptHost.runtimeconfig.json"
            "$<TARGET_FILE_DIR:${target_name}>/managed"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SFG_MANAGED_SCRIPT_HOST_OUTPUT_DIRECTORY}/Stakeforge.Managed.dll"
            "$<TARGET_FILE_DIR:${target_name}>/managed")
    endif()

    add_custom_command(
    TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${sfg_core_dir}/deps/dx12_bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>")

    add_custom_command(
    TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${sfg_core_dir}/deps/pix/bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIGURATION>")

endfunction()
