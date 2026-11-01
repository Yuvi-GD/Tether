if(NOT DEFINED SOURCE_ROOT OR NOT DEFINED IDENTITY_HEADER)
    message(FATAL_ERROR "Source identity requires SOURCE_ROOT and IDENTITY_HEADER")
endif()

# A stable, path-qualified digest includes uncommitted edits and new source
# files. This script runs at build time; it does not alter the CMake cache.
file(GLOB_RECURSE identity_files
    "${SOURCE_ROOT}/src/*.c" "${SOURCE_ROOT}/include/*.h"
    "${SOURCE_ROOT}/tests/*.c" "${SOURCE_ROOT}/tests/*.h"
    "${SOURCE_ROOT}/examples/sandbox/*.c" "${SOURCE_ROOT}/examples/sandbox/*.h"
    "${SOURCE_ROOT}/examples/sandbox/*.yaml" "${SOURCE_ROOT}/cmake/*.cmake"
    "${SOURCE_ROOT}/tests/CMakeLists.txt" "${SOURCE_ROOT}/tests/*/CMakeLists.txt")
list(APPEND identity_files
    "${SOURCE_ROOT}/CMakeLists.txt"
    "${SOURCE_ROOT}/examples/CMakeLists.txt"
    "${SOURCE_ROOT}/cmake/TetherConfig.cmake.in"
    "${SOURCE_ROOT}/tests/support/build_info.h.in")
list(REMOVE_DUPLICATES identity_files)
list(SORT identity_files)
set(identity_manifest "")
foreach(source IN LISTS identity_files)
    file(SHA256 "${source}" source_hash)
    file(RELATIVE_PATH source_name "${SOURCE_ROOT}" "${source}")
    string(APPEND identity_manifest "${source_name}:${source_hash}\n")
endforeach()
string(SHA256 source_sha256 "${identity_manifest}")

set(tether_revision "unknown")
set(chitra_revision "unknown")
set(tether_dirty "unknown")
set(chitra_dirty "unknown")
if(GIT_COMMAND AND EXISTS "${GIT_COMMAND}")
    execute_process(COMMAND "${GIT_COMMAND}" rev-parse HEAD
        WORKING_DIRECTORY "${SOURCE_ROOT}"
        OUTPUT_VARIABLE tether_revision OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(COMMAND "${GIT_COMMAND}" rev-parse HEAD
        WORKING_DIRECTORY "${SOURCE_ROOT}/third_party/chitra"
        OUTPUT_VARIABLE chitra_revision OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(COMMAND "${GIT_COMMAND}" status --porcelain --untracked-files=normal
        WORKING_DIRECTORY "${SOURCE_ROOT}"
        OUTPUT_VARIABLE tether_status ERROR_QUIET)
    execute_process(COMMAND "${GIT_COMMAND}" status --porcelain
        WORKING_DIRECTORY "${SOURCE_ROOT}/third_party/chitra"
        OUTPUT_VARIABLE chitra_status ERROR_QUIET)
    if(tether_status STREQUAL "")
        set(tether_dirty "false")
    else()
        set(tether_dirty "true")
    endif()
    if(chitra_status STREQUAL "")
        set(chitra_dirty "false")
    else()
        set(chitra_dirty "true")
    endif()
endif()

set(header "#define TOOL_REVISION \"${tether_revision}\"\n")
string(APPEND header "#define TOOL_CHITRA_REVISION \"${chitra_revision}\"\n")
string(APPEND header "#define TOOL_CHITRA_DIRTY \"${chitra_dirty}\"\n")
string(APPEND header "#define TOOL_SOURCE_SHA256 \"${source_sha256}\"\n")
string(APPEND header "#define TOOL_SOURCE_DIRTY \"${tether_dirty}\"\n")
if(EXISTS "${IDENTITY_HEADER}")
    file(READ "${IDENTITY_HEADER}" previous_header)
else()
    set(previous_header "")
endif()
if(NOT header STREQUAL previous_header)
    file(WRITE "${IDENTITY_HEADER}" "${header}")
endif()
