add_library(xaimassist_gameplay
  src/BuiltInModes.cpp
  src/GameMode.cpp
  src/GameModeRegistry.cpp
  src/ModeOrchestrator.cpp
  src/RandomEngineFactory.cpp
  src/TargetFactory.cpp
  src/TargetSystem.cpp
  src/modes/GridShotMode.cpp
  src/modes/NextShotMode.cpp
  src/modes/StaticSphereMode.cpp
  src/modes/StrafingTargetsMode.cpp
  src/modes/TrackingTargetsMode.cpp
  src/targets/Target.cpp
)

target_link_libraries(xaimassist_gameplay
  PUBLIC
    xaimassist_core
)

xaimassist_configure_module_library(xaimassist_gameplay)
