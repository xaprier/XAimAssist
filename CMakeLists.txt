cmake_minimum_required(VERSION 3.24)

project(XAimAssist VERSION 0.1.0 LANGUAGES C CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

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

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Sql Qml Quick QuickWidgets QuickControls2)

find_package(VTK 9.2 REQUIRED COMPONENTS
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