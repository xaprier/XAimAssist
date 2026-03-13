add_library(xaimassist_engine
  src/CameraBackend.cpp
  src/Engine.cpp
  src/RenderContext.cpp
  src/Scene.cpp
)

target_link_libraries(xaimassist_engine
  PUBLIC
    xaimassist_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    VTK::CommonCore
    VTK::FiltersSources
    VTK::GUISupportQt
    VTK::InteractionStyle
    VTK::RenderingCore
    VTK::RenderingOpenGL2
)

xaimassist_configure_module_library(xaimassist_engine)


