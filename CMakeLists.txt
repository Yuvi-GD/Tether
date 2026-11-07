cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(TetherUI VERSION 0.0.2 LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(TETHER_STANDALONE OFF)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(TETHER_STANDALONE ON)
endif()
option(TETHER_BUILD_SHARED "Build a shared Tether library" ON)
option(TETHER_BUILD_EXAMPLES "Build the sandbox application" ${TETHER_STANDALONE})
option(TETHER_BUILD_TESTS "Build and register correctness tests" ${TETHER_STANDALONE})
# Compatibility with existing build directories; new configurations use RUNNERS.
if(DEFINED TETHER_BUILD_TOOLS AND NOT DEFINED TETHER_BUILD_RUNNERS)
    set(TETHER_BUILD_RUNNERS ${TETHER_BUILD_TOOLS} CACHE BOOL "Build standalone test and benchmark runners")
endif()
option(TETHER_BUILD_RUNNERS "Build standalone test and benchmark runners" ${TETHER_STANDALONE})
option(TETHER_ENABLE_INSTALL "Enable installation and CMake package exports" ${TETHER_STANDALONE})
include(cmake/TetherSanitizers.cmake)

include(GNUInstallDirs)
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
set(INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/yaml")
add_subdirectory("${THIRD_PARTY_DIR}/libyaml")
add_subdirectory("${THIRD_PARTY_DIR}/chitra")
set(TETHER_SOURCES
    src/tether.c
    src/core/tether_ecs.c
    src/core/tether_registry.c
    src/core/tether_input.c
    src/core/tether_events.c
    src/utils/tether_hashmap.c
    src/backends/chitra_backend.c
    src/engine/parser/tether_yaml.c
    src/engine/parser/tether_yaml_ast.c
    src/engine/tether_engine.c
    src/engine/tether_layout.c
    src/ui/tether_widgets.c
    src/ui/tether_ui.c
)
# Compile the engine once. The DLL and isolated runner/application binaries
# reuse these objects; each operating-system process owns its runtime state.
set(TETHER_PLATFORM_LIBRARIES)
if(WIN32)
    set(TETHER_PLATFORM_LIBRARIES dwmapi)
elseif(APPLE)
    set(TETHER_PLATFORM_LIBRARIES "-framework Cocoa" "-framework QuartzCore" "-framework Metal" "-framework MetalKit")
else()
    set(TETHER_PLATFORM_LIBRARIES X11 Xi Xcursor GL pthread dl m)
endif()
add_library(tether_build_dependencies INTERFACE)
target_include_directories(tether_build_dependencies INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(tether_build_dependencies INTERFACE yaml chitra ${TETHER_PLATFORM_LIBRARIES})
add_library(tether_objects OBJECT ${TETHER_SOURCES})
target_link_libraries(tether_objects PRIVATE tether_build_dependencies)
if(TETHER_BUILD_SHARED)
    set_target_properties(tether_objects yaml chitra PROPERTIES POSITION_INDEPENDENT_CODE ON)
    add_library(tether SHARED $<TARGET_OBJECTS:tether_objects>)
    set_target_properties(tether PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    add_library(tether STATIC $<TARGET_OBJECTS:tether_objects>)
endif()
add_library(Tether::tether ALIAS tether)
target_include_directories(tether PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(tether PRIVATE $<BUILD_INTERFACE:tether_build_dependencies>
    $<INSTALL_INTERFACE:yaml> $<INSTALL_INTERFACE:Tether::chitra> ${TETHER_PLATFORM_LIBRARIES})

if(TETHER_BUILD_EXAMPLES OR TETHER_BUILD_TESTS OR TETHER_BUILD_RUNNERS)
    add_library(tether_runtime STATIC $<TARGET_OBJECTS:tether_objects>)
    target_link_libraries(tether_runtime PUBLIC tether_build_dependencies)
    if(TETHER_BUILD_TESTS)
        enable_testing()
    endif()
    add_subdirectory(tests)
endif()
if(TETHER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
include(cmake/TetherInstall.cmake)
message(STATUS "Tether ${PROJECT_VERSION}: shared=${TETHER_BUILD_SHARED}, sandbox=${TETHER_BUILD_EXAMPLES}, tests=${TETHER_BUILD_TESTS}, runners=${TETHER_BUILD_RUNNERS}")
