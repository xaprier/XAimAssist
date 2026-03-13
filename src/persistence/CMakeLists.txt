add_library(xaimassist_persistence
  src/PersistenceDatabase.cpp
  src/ProfileManager.cpp
  src/SessionHistory.cpp
  src/SettingsManager.cpp
)

target_link_libraries(xaimassist_persistence
  PUBLIC
    xaimassist_core
    Qt6::Core
    Qt6::Sql
)

xaimassist_configure_module_library(xaimassist_persistence)
