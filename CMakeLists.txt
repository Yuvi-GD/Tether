cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(TetherUI C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Paths
# ============================================================
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")

# ============================================================
# Step 1: Add libyaml and chitra from third_party
# ============================================================
message(STATUS "[Tether] Adding libyaml from third_party...")
add_subdirectory(${THIRD_PARTY_DIR}/libyaml)
message(STATUS "[Tether] Adding chitra from third_party...")
add_subdirectory(${THIRD_PARTY_DIR}/chitra)

# ============================================================
# Step 2: Define Engine (tether.dll) and Sandbox
# ============================================================

# 2.1: The Core Engine Library (Shared / DLL)
add_library(tether SHARED
    src/tether.c
    src/core/tether_ecs.c
    src/core/tether_registry.c
    src/core/tether_input.c
    src/core/tether_events.c
    src/utils/tether_hashmap.c
    src/backends/chitra_backend.c
    src/engine/tether_render.c
    src/engine/parser/tether_yaml.c
    src/engine/parser/tether_yaml_ast.c
    src/engine/tether_engine.c
    src/engine/tether_layout.c
    src/ui/tether_widgets.c
)
# Automatically export all symbols on Windows
set_target_properties(tether PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

# 2.2: The Sandbox Application
add_executable(tether_sandbox
    examples/sandbox/sandbox_main.c
    examples/sandbox/sandbox_events.c
    examples/sandbox/sandbox_widget.c
)

# 2.3: ECS Unit Test
add_executable(tether_ecs_test
    tests/unit/ecs_test.c
    src/core/tether_ecs.c
    src/core/tether_registry.c
    src/utils/tether_hashmap.c
)

# ============================================================
# Step 3: Include Paths
# ============================================================
target_include_directories(tether PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src"
)

target_include_directories(tether_sandbox PRIVATE
    "${CMAKE_SOURCE_DIR}/examples/sandbox/include"
)

target_include_directories(tether_ecs_test PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src"
)

# ============================================================
# Step 4: Link Libraries
# ============================================================
target_link_libraries(tether PRIVATE yaml chitra)
target_link_libraries(tether_sandbox PRIVATE tether)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(tether PRIVATE X11 Xi Xcursor GL pthread dl m)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(tether PRIVATE dwmapi)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(tether PRIVATE "-framework Cocoa" "-framework QuartzCore" "-framework Metal" "-framework MetalKit")
endif()

# ============================================================
# Step 5: Post-Build Asset Copying
# ============================================================
add_custom_command(TARGET tether_sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/examples/UI"
        "$<TARGET_FILE_DIR:tether_sandbox>/UI"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/examples/Font"
        "$<TARGET_FILE_DIR:tether_sandbox>/Font"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:tether>"
        "$<TARGET_FILE_DIR:tether_sandbox>"
    COMMENT "Copying assets (UI, Font) to build directory..."
)

# ============================================================
# Summary
# ============================================================
message(STATUS "========================================")
message(STATUS "  Tether Build Configuration (Chitra)")
message(STATUS "========================================")
message(STATUS "  OS:          ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "  Backend:     Chitra (Sokol D3D11/Metal/GL)")
message(STATUS "========================================")
