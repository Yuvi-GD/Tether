cmake_minimum_required(VERSION 3.20)
project(TetherUI C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Version Pins
# ============================================================
set(WGPU_VERSION "v29.0.0.0")

# ============================================================
# Paths
# ============================================================
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")
set(WGPU_DIR        "${THIRD_PARTY_DIR}/webgpu")
set(SOKOL_DIR       "${THIRD_PARTY_DIR}/sokol")
set(THORVG_DIR      "${THIRD_PARTY_DIR}/thorvg")
set(THORVG_BUILDDIR "${THORVG_DIR}/builddir")

# ============================================================
# Step 1: Download wgpu-native pre-built binaries
# ============================================================
include(FetchContent)

if(NOT EXISTS "${WGPU_DIR}/include/webgpu/webgpu.h")
    message(STATUS "[Tether] Downloading wgpu-native ${WGPU_VERSION} for ${CMAKE_SYSTEM_NAME}...")

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64")
            set(WGPU_URL "https://github.com/gfx-rs/wgpu-native/releases/download/${WGPU_VERSION}/wgpu-windows-aarch64-msvc-release.zip")
        else()
            set(WGPU_URL "https://github.com/gfx-rs/wgpu-native/releases/download/${WGPU_VERSION}/wgpu-windows-x86_64-msvc-release.zip")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            set(WGPU_URL "https://github.com/gfx-rs/wgpu-native/releases/download/${WGPU_VERSION}/wgpu-linux-aarch64-gnu-release.zip")
        else()
            set(WGPU_URL "https://github.com/gfx-rs/wgpu-native/releases/download/${WGPU_VERSION}/wgpu-linux-x86_64-gnu-release.zip")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(WGPU_URL "https://github.com/gfx-rs/wgpu-native/releases/download/${WGPU_VERSION}/wgpu-macos-aarch64-release.zip")
    else()
        message(FATAL_ERROR
            "[Tether] Unsupported OS: ${CMAKE_SYSTEM_NAME}.\n"
            "Pre-built wgpu-native binaries are not available for this platform.\n"
            "Please build wgpu-native from source: https://github.com/gfx-rs/wgpu-native")
    endif()

    FetchContent_Declare(
        wgpu_native_download
        URL "${WGPU_URL}"
        SOURCE_DIR "${WGPU_DIR}"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(wgpu_native_download)
    message(STATUS "[Tether] wgpu-native downloaded to ${WGPU_DIR}")
else()
    message(STATUS "[Tether] wgpu-native already present, skipping download.")
endif()

# ============================================================
# Step 2: Resolve wgpu-native library paths
# ============================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(WGPU_IMPLIB   "${WGPU_DIR}/lib/wgpu_native.dll.lib")
    set(WGPU_SHARED   "${WGPU_DIR}/lib/wgpu_native.dll")
    set(WGPU_LINK_FLAGS "-lwgpu_native")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(WGPU_IMPLIB   "${WGPU_DIR}/lib/libwgpu_native.so")
    set(WGPU_SHARED   "${WGPU_DIR}/lib/libwgpu_native.so")
    set(WGPU_LINK_FLAGS "-lwgpu_native")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(WGPU_IMPLIB   "${WGPU_DIR}/lib/libwgpu_native.dylib")
    set(WGPU_SHARED   "${WGPU_DIR}/lib/libwgpu_native.dylib")
    set(WGPU_LINK_FLAGS "-lwgpu_native")
endif()

if(NOT EXISTS "${WGPU_IMPLIB}")
    message(FATAL_ERROR
        "[Tether] wgpu-native library not found at: ${WGPU_IMPLIB}\n"
        "Check third_party/webgpu/lib/ and update WGPU_IMPLIB in CMakeLists.txt.")
endif()

# ============================================================
# Step 3: Build ThorVG with Meson
# ============================================================
set(WGPU_CMAKE_DIR "${WGPU_DIR}/lib/cmake/wgpu_native")
file(MAKE_DIRECTORY "${WGPU_CMAKE_DIR}")
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/wgpu_nativeConfig.cmake.in"
    "${WGPU_CMAKE_DIR}/wgpu_nativeConfig.cmake"
    @ONLY
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(THORVG_LIB_NAME "thorvg-1.lib")
    set(THORVG_DLL_NAME "thorvg-1.dll")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(THORVG_LIB_NAME "libthorvg.1.dylib")
    set(THORVG_DLL_NAME "libthorvg.1.dylib")
else()
    set(THORVG_LIB_NAME "libthorvg.so.1")
    set(THORVG_DLL_NAME "libthorvg.so.1")
endif()

set(THORVG_LIB "${THORVG_BUILDDIR}/src/${THORVG_LIB_NAME}")

find_program(MESON_EXE  NAMES meson meson.exe  REQUIRED)
find_program(NINJA_EXE  NAMES ninja ninja.exe  REQUIRED)

if(NOT EXISTS "${THORVG_LIB}")
    message(STATUS "[Tether] Building ThorVG with Meson...")

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "CMAKE_PREFIX_PATH=${WGPU_DIR}"
            "${MESON_EXE}" setup "${THORVG_BUILDDIR}"
                --reconfigure
                --cmake-prefix-path "${WGPU_DIR}"
                -Dcpp_std=c++20
                -Dengines=wg
                -Dbindings=capi
                -Dloaders=svg,lottie,ttf,png,jpg
                -Dextra=lottie_exp,openmp
                -Dpartial=true
                -Dthreads=true
        WORKING_DIRECTORY "${THORVG_DIR}"
        RESULT_VARIABLE MESON_SETUP_RESULT
        COMMAND_ECHO STDOUT
    )

    if(NOT MESON_SETUP_RESULT EQUAL 0)
        message(FATAL_ERROR "[Tether] Meson setup failed.")
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "CMAKE_PREFIX_PATH=${WGPU_DIR}"
            "${MESON_EXE}" compile -C "${THORVG_BUILDDIR}"
        WORKING_DIRECTORY "${THORVG_DIR}"
        RESULT_VARIABLE MESON_BUILD_RESULT
        COMMAND_ECHO STDOUT
    )

    if(NOT MESON_BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "[Tether] ThorVG Meson build failed.")
    endif()

    message(STATUS "[Tether] ThorVG built successfully: ${THORVG_LIB}")
else()
    message(STATUS "[Tether] ThorVG already built, skipping.")
endif()

# ============================================================
# Step 4: Define Executable Targets
# ============================================================
add_executable(tether_sandbox
    tests/sandbox_main.c
    src/tether.c
    src/backends/hal/sokol_hal.c
    src/backends/raster/thorvg_raster.c
)

# ============================================================
# Step 5: Include Paths
# ============================================================
target_include_directories(tether_sandbox PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src"
    "${SOKOL_DIR}"
    "${THORVG_DIR}/inc"
    "${THORVG_DIR}/src/bindings/capi"
    "${THORVG_BUILDDIR}"
    "${WGPU_DIR}/include"
)

# ============================================================
# Step 6: Link Libraries
# ============================================================
target_link_directories(tether_sandbox PRIVATE
    "${WGPU_DIR}/lib"
)

target_link_libraries(tether_sandbox PRIVATE
    "${THORVG_LIB}"
    "${WGPU_IMPLIB}"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(tether_sandbox PRIVATE
        X11 Xi Xcursor GL pthread dl m
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(tether_sandbox PRIVATE
        dwmapi
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(tether_sandbox PRIVATE
        "-framework Cocoa"
        "-framework QuartzCore"
        "-framework Metal"
        "-framework MetalKit"
    )
endif()

# ============================================================
# Step 7: Runtime Library Copying
# ============================================================
set(RUNTIME_LIBS_TO_COPY "")

if(EXISTS "${WGPU_SHARED}")
    list(APPEND RUNTIME_LIBS_TO_COPY "${WGPU_SHARED}")
endif()

if(EXISTS "${THORVG_BUILDDIR}/src/${THORVG_DLL_NAME}")
    list(APPEND RUNTIME_LIBS_TO_COPY "${THORVG_BUILDDIR}/src/${THORVG_DLL_NAME}")
endif()

foreach(LIB_FILE ${RUNTIME_LIBS_TO_COPY})
    add_custom_command(TARGET tether_sandbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIB_FILE}"
            $<TARGET_FILE_DIR:tether_sandbox>
        COMMENT "Copying ${LIB_FILE} next to executable"
    )
endforeach()

# ============================================================
# Summary
# ============================================================
message(STATUS "========================================")
message(STATUS "  Tether Build Configuration")
message(STATUS "========================================")
message(STATUS "  OS:          ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "  wgpu-native: ${WGPU_VERSION}")
message(STATUS "  ThorVG lib:  ${THORVG_LIB}")
message(STATUS "  wgpu lib:    ${WGPU_IMPLIB}")
message(STATUS "========================================")