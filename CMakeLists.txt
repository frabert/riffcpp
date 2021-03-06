cmake_minimum_required(VERSION 3.1)
project(riffcpp VERSION 2.2.4
                DESCRIPTION "Simple library for reading RIFF files"
                LANGUAGES CXX)

add_library(riffcpp "")
add_library(RiffCpp::riffcpp ALIAS riffcpp)

set(RIFFCPP_EXAMPLE_INSTALL_DIR "bin" CACHE "PATH" "Where to install the example executable")
option(RIFFCPP_INSTALL_EXAMPLE "Install example executable" ON)
option(RIFFCPP_INSTALL_HEADERS "Install development headers" ON)
option(RIFFCPP_INSTALL_LIB "Install library binary" ON)
option(RIFFCPP_BIGENDIAN "Whether to build as if the target system is Big Endian" OFF)

include(GenerateExportHeader)
generate_export_header(riffcpp)

target_sources(riffcpp
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/riffcpp.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/membuffer.cpp
)

target_compile_features(riffcpp PUBLIC cxx_std_14)
target_include_directories(riffcpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

if(RIFFCPP_BIGENDIAN)
    target_compile_definitions(riffcpp PRIVATE RIFFCPP_BIGENDIAN)
endif()

if(RIFFCPP_INSTALL_HEADERS)
    target_include_directories(riffcpp PUBLIC $<INSTALL_INTERFACE:include>)
endif()

add_executable(riff_dump ${CMAKE_CURRENT_LIST_DIR}/src/test.cpp)
target_link_libraries(riff_dump PRIVATE riffcpp)

include(GNUInstallDirs)

# install library files needed for linking
if(RIFFCPP_INSTALL_LIB)
    install(
        TARGETS riffcpp
        EXPORT riffcpp-targets
        COMPONENT runtime
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(RIFFCPP_INSTALL_HEADERS)
    # install the public header files
    install(
        DIRECTORY ${CMAKE_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT devel
    )

    install(
        FILES ${PROJECT_BINARY_DIR}/riffcpp_export.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT devel
    )

    # installs the *cmake files in share directory
    install(
        EXPORT riffcpp-targets
        FILE riffcpp-targets.cmake
        NAMESPACE libdmusic::
        DESTINATION share/riffcpp
        COMPONENT devel
    )

    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(
        ${CMAKE_BINARY_DIR}/cmake/riffcpp-config-version.cmake
        VERSION ${RIFFCPP_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/riffcpp-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/riffcpp-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/riffcpp
    )

    install(
        FILES
            ${CMAKE_BINARY_DIR}/cmake/riffcpp-config.cmake
            ${CMAKE_BINARY_DIR}/cmake/riffcpp-config-version.cmake
        DESTINATION share/riffcpp
        COMPONENT devel
    )
endif()

if(RIFFCPP_INSTALL_EXAMPLE)
    install(
        TARGETS riff_dump
        RUNTIME DESTINATION ${RIFFCPP_EXAMPLE_INSTALL_DIR}
        COMPONENT example
    )
endif()

#Register package in user's package registry
export(PACKAGE riffcpp)

# Packaging support
set(CPACK_PACKAGE_VENDOR "Francesco Bertolaccini")
set(CPACK_PACKAGE_CONTACT "francesco@bertolaccini.dev")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple library for reading RIFF files")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "libriffcpp")
set(CPACK_DEBIAN_DEVEL_PACKAGE_NAME "libriffcpp-dev")
set(CPACK_DEBIAN_EXAMPLE_PACKAGE_NAME "libriffcpp-utils")
set(CPACK_DEBIAN_RUNTIME_FILE_NAME libriffcpp_${CPACK_PACKAGE_VERSION}.deb)
set(CPACK_DEBIAN_DEVEL_FILE_NAME libriffcpp-dev_${CPACK_PACKAGE_VERSION}.deb)
set(CPACK_DEBIAN_EXAMPLE_FILE_NAME libriffcpp-utils_${CPACK_PACKAGE_VERSION}.deb)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.14), libstdc++6 (>= 5.2), libgcc1 (>= 1:3.0)")
set(CPACK_DEBIAN_DEVEL_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libriffcpp (= ${CPACK_PACKAGE_VERSION})")
set(CPACK_DEBIAN_EXAMPLE_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libriffcpp (= ${CPACK_PACKAGE_VERSION})")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)

if(RIFFCPP_INSTALL_HEADERS)
    cpack_add_component(
        devel
        DISPLAY_NAME "Header files"
        DESCRIPTION "Required if you need to compile programs that depend on this library"
        DEPENDS runtime
    )
endif()

if(RIFFCPP_INSTALL_LIB)
    cpack_add_component(
        runtime
        DISPLAY_NAME "Runtime library"
        REQUIRED
    )
endif()

if(RIFFCPP_INSTALL_EXAMPLE)
    cpack_add_component(
        example
        DISPLAY_NAME "Example program"
        DEPENDS runtime
    )
endif()

include(CTest)
add_subdirectory(tests)