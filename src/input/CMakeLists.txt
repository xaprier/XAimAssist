add_library(xaimassist_input
  src/InputManager.cpp
  src/SensitivityModel.cpp
)

target_link_libraries(xaimassist_input
  PUBLIC
    xaimassist_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

xaimassist_configure_module_library(xaimassist_input)
