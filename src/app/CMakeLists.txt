set(XAIMASSIST_APP_TARGET xaimassist_app)

set(XAIMASSIST_WINDOWS_ICON_RESOURCE)

if(WIN32)
    set(XAIMASSIST_WINDOWS_ICON_RESOURCE
        ${CMAKE_CURRENT_BINARY_DIR}/xaimassist_icon.rc
    )

    configure_file(
        ${PROJECT_SOURCE_DIR}/assets/icon.rc
        ${XAIMASSIST_WINDOWS_ICON_RESOURCE}
        COPYONLY
    )

    configure_file(
        ${PROJECT_SOURCE_DIR}/assets/XAimAssist.ico
        ${CMAKE_CURRENT_BINARY_DIR}/XAimAssist.ico
        COPYONLY
    )
endif()

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


if(WIN32)
    target_sources(${XAIMASSIST_APP_TARGET}
        PRIVATE
            ${XAIMASSIST_WINDOWS_ICON_RESOURCE}
    )
    set_target_properties(${XAIMASSIST_APP_TARGET}
        PROPERTIES
            WIN32_EXECUTABLE ON
    )
endif()

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

install(
    TARGETS ${XAIMASSIST_APP_TARGET}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(
        FILES
            ${PROJECT_SOURCE_DIR}/assets/XAimAssist.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )

    install(
        FILES
            ${PROJECT_SOURCE_DIR}/assets/XAimAssist.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
    )
endif()

if(WIN32)
    install(
        FILES
            ${PROJECT_SOURCE_DIR}/assets/XAimAssist.ico
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
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
