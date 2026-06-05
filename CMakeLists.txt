cmake_minimum_required(VERSION 3.15)
project(TetherUI)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# Dependency Management (FetchContent)
# ---------------------------------------------------------
include(FetchContent)

# Fetch Sokol (Header only)
FetchContent_Declare(
  sokol
  GIT_REPOSITORY https://github.com/floooh/sokol.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(sokol)

# ---------------------------------------------------------
# 5. Define the Sandbox Executable
# ---------------------------------------------------------
include_directories(include)
include_directories(${sokol_SOURCE_DIR})
include_directories(${thorvg_SOURCE_DIR}/inc)
include_directories(${thorvg_SOURCE_DIR}/src/bindings/capi)

add_executable(tether_sandbox 
    tests/sandbox_main.c
    src/backends/tether_backend.c
)

# Explicitly tell the compiler where to find all required header files (.h)
target_include_directories(tether_sandbox PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${sokol_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/third_party/thorvg/inc"
    "${CMAKE_SOURCE_DIR}/third_party/thorvg/src/bindings/capi"
)

# Link the library that Meson just built for us!
target_link_libraries(tether_sandbox PRIVATE "${CMAKE_SOURCE_DIR}/third_party/thorvg/builddir/src/thorvg-1.lib")

# Automatically copy the ThorVG DLL to the executable's output directory after building
add_custom_command(TARGET tether_sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/third_party/thorvg/builddir/src/thorvg-1.dll"
    $<TARGET_FILE_DIR:tether_sandbox>
)