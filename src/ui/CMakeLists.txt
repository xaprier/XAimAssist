add_library(xaimassist_ui
	include/ui/UiViewModel.hpp
	src/UiViewModel.cpp
	resources/ui.qrc
)

target_include_directories(xaimassist_ui
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

# Configure version header
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/ui")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ui/Version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/ui/Version.hpp
    @ONLY
)

target_link_libraries(xaimassist_ui
	PUBLIC
		xaimassist_core
		xaimassist_persistence
		Qt6::Core
		Qt6::Gui
		Qt6::Qml
		Qt6::Quick
		Qt6::QuickWidgets
		Qt6::QuickControls2
)

target_compile_features(xaimassist_ui PUBLIC cxx_std_17)
