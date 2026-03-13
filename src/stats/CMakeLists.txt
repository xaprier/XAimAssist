add_library(xaimassist_stats
  src/StatTracker.cpp
)

target_link_libraries(xaimassist_stats
  PUBLIC
    xaimassist_core
)

xaimassist_configure_module_library(xaimassist_stats)
