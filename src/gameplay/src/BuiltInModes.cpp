/// @file BuiltInModes.cpp
#include "gameplay/BuiltInModes.hpp"

#include <memory>

#include "gameplay/GameModeRegistry.hpp"
#include "gameplay/SettingsProvider.hpp"
#include "gameplay/modes/GridShotMode.hpp"
#include "gameplay/modes/NextShotMode.hpp"
#include "gameplay/modes/StaticSphereMode.hpp"
#include "gameplay/modes/StrafingTargetsMode.hpp"
#include "gameplay/modes/TrackingTargetsMode.hpp"

namespace xaimassist::gameplay {
void RegisterBuiltInModes(GameModeRegistry& registry) {
    registry.RegisterMode(
        StaticSphereMode::MetadataDefinition(),
        [](core::EventBus& eventBus, core::Logger& logger,
           ISceneCommandSink& sceneCommandSink, TargetSystem& targetSystem,
           IGameplaySettingsProvider& settingsProvider) {
            return std::make_unique<StaticSphereMode>(
                eventBus, logger, sceneCommandSink, targetSystem, settingsProvider);
        });

    registry.RegisterMode(
        GridShotMode::MetadataDefinition(),
        [](core::EventBus& eventBus, core::Logger& logger,
           ISceneCommandSink& sceneCommandSink, TargetSystem& targetSystem,
           IGameplaySettingsProvider& settingsProvider) {
            return std::make_unique<GridShotMode>(
                eventBus, logger, sceneCommandSink, targetSystem, settingsProvider);
        });

    registry.RegisterMode(
        NextShotMode::MetadataDefinition(),
        [](core::EventBus& eventBus, core::Logger& logger,
           ISceneCommandSink& sceneCommandSink, TargetSystem& targetSystem,
           IGameplaySettingsProvider& settingsProvider) {
            return std::make_unique<NextShotMode>(
                eventBus, logger, sceneCommandSink, targetSystem, settingsProvider);
        });

    registry.RegisterMode(
        StrafingTargetsMode::MetadataDefinition(),
        [](core::EventBus& eventBus, core::Logger& logger,
           ISceneCommandSink& sceneCommandSink, TargetSystem& targetSystem,
           IGameplaySettingsProvider& settingsProvider) {
            return std::make_unique<StrafingTargetsMode>(
                eventBus, logger, sceneCommandSink, targetSystem, settingsProvider);
        });

    registry.RegisterMode(
        TrackingTargetsMode::MetadataDefinition(),
        [](core::EventBus& eventBus, core::Logger& logger,
           ISceneCommandSink& sceneCommandSink, TargetSystem& targetSystem,
           IGameplaySettingsProvider& settingsProvider) {
            return std::make_unique<TrackingTargetsMode>(
                eventBus, logger, sceneCommandSink, targetSystem, settingsProvider);
        });
}
}  // namespace xaimassist::gameplay
