set(XAIMASSIST_APP_TARGET xaimassist_app)

add_executable(${XAIMASSIST_APP_TARGET}
    src/Application.cpp
    src/AppGameplaySettingsProvider.cpp
    src/CompositionRoot.cpp
    src/EngineRenderBridge.cpp
    src/GameplayWorldBridge.cpp
    src/PerformanceClassifier.cpp
    src/main.cpp
    src/RuntimeLoop.cpp
    src/SettingsConverter.cpp
    src/WorldRaycastHitTest.cpp
)

target_include_directories(${XAIMASSIST_APP_TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${XAIMASSIST_APP_TARGET}
    PRIVATE
        xaimassist_core
        xaimassist_engine
        xaimassist_world
        xaimassist_gameplay
        xaimassist_input
        xaimassist_stats
        xaimassist_persistence
        xaimassist_ui
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::QuickWidgets
        Qt6::Widgets
)

set_target_properties(${XAIMASSIST_APP_TARGET}
    PROPERTIES
        OUTPUT_NAME XAimAssist
)

if(UNIX AND NOT APPLE)
    set_target_properties(${XAIMASSIST_APP_TARGET}
        PROPERTIES
            INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
    )
endif()

if(WIN32 AND XAIMASSIST_ENABLE_DEPLOYMENT)
    install(
        TARGETS ${XAIMASSIST_APP_TARGET}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION .
        RUNTIME_DEPENDENCY_SET xaimassist_app_runtime_dependencies
    )

    install(
        RUNTIME_DEPENDENCY_SET xaimassist_app_runtime_dependencies
        PRE_EXCLUDE_REGEXES
            "api-ms-win-.*"
            "ext-ms-.*"
            ".*[\\\\/](Qt6|libQt6).*"
        POST_EXCLUDE_REGEXES
            ".*[\\\\/]system32[\\\\/].*\\.dll"
            ".*[\\\\/]Windows[\\\\/].*\\.dll"
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
else()
    install(
        TARGETS ${XAIMASSIST_APP_TARGET}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION .
    )
endif()

if(XAIMASSIST_ENABLE_DEPLOYMENT AND COMMAND qt_generate_deploy_app_script)
    qt_generate_deploy_app_script(
        TARGET ${XAIMASSIST_APP_TARGET}
        OUTPUT_SCRIPT XAIMASSIST_QT_DEPLOY_SCRIPT
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${XAIMASSIST_QT_DEPLOY_SCRIPT})
endif()

vtk_module_autoinit(
    TARGETS ${XAIMASSIST_APP_TARGET}
    MODULES
        VTK::CommonCore
        VTK::FiltersSources
        VTK::GUISupportQt
        VTK::InteractionStyle
        VTK::RenderingCore
        VTK::RenderingOpenGL2
)
