# ==============================================================================
# Tether WebAssembly Configuration Helper
# ==============================================================================
# Usage:
#   tether_configure_wasm_target(<target>
#       [SHELL <path_to_custom_shell.html>]
#       [PRELOAD_DIRS <dir1@mount1> <dir2@mount2> ...]
#       [EXPORTED_FUNCTIONS <fn1> <fn2> ...]
#       [EXPORTED_METHODS <method1> <method2> ...]
#   )
# ==============================================================================

function(tether_configure_wasm_target TARGET_NAME)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        return()
    endif()

    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "SHELL" "PRELOAD_DIRS;EXPORTED_FUNCTIONS;EXPORTED_METHODS")

    if(NOT ARG_SHELL)
        set(ARG_SHELL "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../web/shell.html")
    endif()

    set(WASM_LINK_OPTIONS
        "--shell-file=${ARG_SHELL}"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sWASM=1"
        "-sSINGLE_FILE=1"
    )

    # Standard runtime methods needed by Tether platform and virtual filesystem
    set(DEFAULT_METHODS "stringToUTF8,stringToNewUTF8,UTF8ToString,FS")
    if(ARG_EXPORTED_METHODS)
        string(JOIN "," CUSTOM_METHODS ${ARG_EXPORTED_METHODS})
        set(DEFAULT_METHODS "${DEFAULT_METHODS},${CUSTOM_METHODS}")
    endif()
    list(APPEND WASM_LINK_OPTIONS "-sEXPORTED_RUNTIME_METHODS=${DEFAULT_METHODS}")

    # Standard exported functions
    set(DEFAULT_FUNCTIONS "_main,_free,_malloc")
    if(ARG_EXPORTED_FUNCTIONS)
        string(JOIN "," CUSTOM_FUNCTIONS ${ARG_EXPORTED_FUNCTIONS})
        set(DEFAULT_FUNCTIONS "${DEFAULT_FUNCTIONS},${CUSTOM_FUNCTIONS}")
    endif()
    list(APPEND WASM_LINK_OPTIONS "-sEXPORTED_FUNCTIONS=${DEFAULT_FUNCTIONS}")

    # Configure asset preloading
    foreach(PRELOAD_DIR ${ARG_PRELOAD_DIRS})
        list(APPEND WASM_LINK_OPTIONS "--preload-file=${PRELOAD_DIR}")
    endforeach()

    target_link_options(${TARGET_NAME} PRIVATE ${WASM_LINK_OPTIONS})
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".html")
endfunction()
