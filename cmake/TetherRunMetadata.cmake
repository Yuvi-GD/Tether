# Compiler settings are stable until CMake is reconfigured. Source identity is
# refreshed by the build, so editing C files does not regenerate the build tree.
foreach(flag_var IN ITEMS CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO)
    string(REPLACE "\\" "\\\\" ${flag_var}_ESCAPED "${${flag_var}}")
    string(REPLACE "\"" "\\\"" ${flag_var}_ESCAPED "${${flag_var}_ESCAPED}")
endforeach()
configure_file("${PROJECT_SOURCE_DIR}/tests/support/build_info.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/build_info.h" @ONLY)

find_package(Git QUIET)
add_custom_target(tether_source_identity
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_ROOT=${PROJECT_SOURCE_DIR}"
        "-DIDENTITY_HEADER=${CMAKE_CURRENT_BINARY_DIR}/generated/source_identity.h"
        "-DGIT_COMMAND=${GIT_EXECUTABLE}"
        -P "${PROJECT_SOURCE_DIR}/cmake/TetherGenerateSourceIdentity.cmake"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/generated/source_identity.h"
    VERBATIM)
