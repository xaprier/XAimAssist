cmake_minimum_required(VERSION 3.21)

project(
    XAimAssist
    VERSION 1.0.8
    LANGUAGES C CXX
    DESCRIPTION "An open-source 3D FPS aim training application built with modern C++ architecture, Qt6, and VTK rendering."
    HOMEPAGE_URL "https://github.com/xaprier/XAimAssist"
)

set(PROJECT_ORGANIZATION "xaprier")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(CompilerOptions)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(XAIMASSIST_BUILD_TESTS "Build tests" OFF)
option(XAIMASSIST_ENABLE_DEPLOYMENT "Enable deployment install hooks" ON)

# On Linux, prefer the system Qt6 so it matches the ABI of system VTK.
# The Qt Installer puts an older version earlier in CMAKE_PREFIX_PATH which
# causes link-time symbol mismatches (qt_version_tag@Qt_6.N) when VTK was
# compiled against the system Qt.
if(UNIX AND NOT APPLE AND NOT DEFINED Qt6_DIR AND EXISTS "/usr/lib/cmake/Qt6")
    set(Qt6_DIR "/usr/lib/cmake/Qt6" CACHE PATH "Qt6 CMake dir (system)")
    message(STATUS "XAimAssist: using system Qt6 at ${Qt6_DIR}")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Qml
    Quick
    QuickWidgets
    QuickControls2)

find_package(VTK 9 REQUIRED COMPONENTS
    CommonColor
    CommonCore
    FiltersSources
    GUISupportQt
    InteractionStyle
    RenderingCore
    RenderingOpenGL2
)

add_subdirectory(src)

set(XAIMASSIST_CMAKE_CONFIG_INSTALL_DIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/XAimAssist"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/XAimAssistConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/XAimAssistConfig.cmake"
    INSTALL_DESTINATION "${XAIMASSIST_CMAKE_CONFIG_INSTALL_DIR}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/XAimAssistConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    EXPORT XAimAssistTargets
    FILE XAimAssistTargets.cmake
    NAMESPACE XAimAssist::
    DESTINATION "${XAIMASSIST_CMAKE_CONFIG_INSTALL_DIR}"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/XAimAssistConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/XAimAssistConfigVersion.cmake"
    DESTINATION "${XAIMASSIST_CMAKE_CONFIG_INSTALL_DIR}"
)
