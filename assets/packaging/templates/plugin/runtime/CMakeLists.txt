file(GLOB_RECURSE RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library("${PROJECT_NAME}" SHARED ${RUNTIME_SOURCES})
target_link_libraries("${PROJECT_NAME}" PUBLIC Stakeforge::core)
target_include_directories("${PROJECT_NAME}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties("${PROJECT_NAME}" PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${RUNTIME_SOURCES})

set(_memory_operators "${Stakeforge_DIR}/../include/sfg/core/memory/memory_operators.cpp")
set_source_files_properties("${_memory_operators}" PROPERTIES HEADER_FILE_ONLY FALSE)
target_sources("${PROJECT_NAME}" PRIVATE "${_memory_operators}")
