# ==============================================================================
# Tether Target Assets Deployment Helper
# ==============================================================================
# Deploys asset directories uniformly across Desktop, Web, and Mobile targets.
#
# Usage:
#   tether_target_assets(<target>
#       [DESTINATION <folder>]    # Default: "assets"
#       DIRECTORIES
#           "<source_dir_1>[@<alias_1>]"
#           "<source_dir_2>[@<alias_2>]"
#   )
# ==============================================================================

function(tether_target_assets TARGET_NAME)
    cmake_parse_arguments(ARG "" "DESTINATION" "DIRECTORIES" ${ARGN})
    if(NOT ARG_DESTINATION)
        set(ARG_DESTINATION "assets")
    endif()

    foreach(ITEM ${ARG_DIRECTORIES})
        if(ITEM MATCHES "(.+)@(.+)")
            set(SRC_DIR "${CMAKE_MATCH_1}")
            set(ALIAS "${CMAKE_MATCH_2}")
        else()
            set(SRC_DIR "${ITEM}")
            get_filename_component(ALIAS "${ITEM}" NAME)
        endif()

        if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
            target_link_options(${TARGET_NAME} PRIVATE
                "--embed-file=${SRC_DIR}@${ARG_DESTINATION}/${ALIAS}"
            )
        else()
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>/${ARG_DESTINATION}/${ALIAS}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_DIR}" "$<TARGET_FILE_DIR:${TARGET_NAME}>/${ARG_DESTINATION}/${ALIAS}"
            )
        endif()
    endforeach()
endfunction()
