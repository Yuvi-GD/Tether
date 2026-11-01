if(TETHER_ENABLE_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # A static Tether archive still needs its renderer and parser at link time.
    # Keep their implementation headers out of the installed Tether interface.
    if(NOT TETHER_BUILD_SHARED)
        set_property(TARGET chitra PROPERTY INTERFACE_INCLUDE_DIRECTORIES
            "$<BUILD_INTERFACE:${THIRD_PARTY_DIR}/chitra/include>")
        install(TARGETS chitra EXPORT TetherTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()

    install(TARGETS tether
        EXPORT TetherTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT TetherTargets
        FILE TetherTargets.cmake
        NAMESPACE Tether::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Tether
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TetherConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/TetherConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Tether"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/TetherConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/TetherConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/TetherConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Tether
    )
endif()
