file(GLOB_RECURSE GAME_EDITOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library("${PROJECT_NAME}_editor" SHARED ${GAME_EDITOR_SOURCES})
target_link_libraries("${PROJECT_NAME}_editor" PRIVATE "${PROJECT_NAME}" Stakeforge::editor)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${GAME_EDITOR_SOURCES})

add_custom_command(TARGET "${PROJECT_NAME}_editor" POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E touch "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.plugin_ready"
    VERBATIM
)

set(_memory_operators "${Stakeforge_DIR}/../include/sfg/core/memory/memory_operators.cpp")
set_source_files_properties("${_memory_operators}" PROPERTIES HEADER_FILE_ONLY FALSE)
target_sources("${PROJECT_NAME}_editor" PRIVATE "${_memory_operators}")
