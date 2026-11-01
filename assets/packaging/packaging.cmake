if(NOT WIN32)
    return()
endif()

get_directory_property(SFG_SDK_DEFINITIONS DIRECTORY "${CMAKE_SOURCE_DIR}/stakeforge_core" COMPILE_DEFINITIONS)
get_directory_property(SFG_SDK_OPTIONS DIRECTORY "${CMAKE_SOURCE_DIR}/stakeforge_core" COMPILE_OPTIONS)
set(SFG_SDK_GENERATED "${CMAKE_BINARY_DIR}/packaging/$<CONFIG>")

file(GENERATE OUTPUT "${SFG_SDK_GENERATED}/definitions.txt"
    CONTENT "$<JOIN:$<REMOVE_DUPLICATES:${SFG_SDK_DEFINITIONS};$<TARGET_PROPERTY:stakeforge_core,INTERFACE_COMPILE_DEFINITIONS>>,\n>\n"
    TARGET stakeforge_core)
file(GENERATE OUTPUT "${SFG_SDK_GENERATED}/options.txt"
    CONTENT "$<JOIN:$<REMOVE_DUPLICATES:${SFG_SDK_OPTIONS};$<TARGET_PROPERTY:stakeforge_core,INTERFACE_COMPILE_OPTIONS>>,\n>\n"
    TARGET stakeforge_core)
file(GENERATE OUTPUT "${SFG_SDK_GENERATED}/flags.txt" CONTENT
"${CMAKE_CXX_FLAGS}
$<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}>$<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}>$<$<CONFIG:Profile>:${CMAKE_CXX_FLAGS_PROFILE}>
${CMAKE_SHARED_LINKER_FLAGS}
$<$<CONFIG:Debug>:${CMAKE_SHARED_LINKER_FLAGS_DEBUG}>$<$<CONFIG:Release>:${CMAKE_SHARED_LINKER_FLAGS_RELEASE}>$<$<CONFIG:Profile>:${CMAKE_SHARED_LINKER_FLAGS_PROFILE}>
${CMAKE_EXE_LINKER_FLAGS}
$<$<CONFIG:Debug>:${CMAKE_EXE_LINKER_FLAGS_DEBUG}>$<$<CONFIG:Release>:${CMAKE_EXE_LINKER_FLAGS_RELEASE}>$<$<CONFIG:Profile>:${CMAKE_EXE_LINKER_FLAGS_PROFILE}>
")

set(SFG_SDK_PLATFORM "${CMAKE_VS_PLATFORM_NAME}")
if(NOT SFG_SDK_PLATFORM)
    set(SFG_SDK_PLATFORM "${CMAKE_SYSTEM_PROCESSOR}")
endif()

configure_file("${CMAKE_CURRENT_LIST_DIR}/sdk_build.json.in" "${CMAKE_BINARY_DIR}/packaging/sdk_build.json.in" @ONLY)
file(GENERATE OUTPUT "${SFG_SDK_GENERATED}/sdk_build.json"
    INPUT "${CMAKE_BINARY_DIR}/packaging/sdk_build.json.in")

add_custom_target(stakeforge_editor_packer
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/assets/packaging/pack_editor.py"
        --manifest "${SFG_SDK_GENERATED}/sdk_build.json"
    DEPENDS stakeforge_core stakeforge_tools
    COMMENT "Packaging editor assets, SDK and runtime binaries"
    VERBATIM)
set_target_properties(stakeforge_editor_packer PROPERTIES FOLDER "stakeforge")
add_dependencies(stakeforge_editor stakeforge_editor_packer)
