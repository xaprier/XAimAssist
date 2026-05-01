/// @file CompositionRoot.cpp
#include "app/CompositionRoot.hpp"

#include "core/EventBus.hpp"
#include "core/FrameClock.hpp"
#include "core/Logger.hpp"
#include "engine/Engine.hpp"
#include "input/InputManager.hpp"
#include "persistence/PersistenceDatabase.hpp"
#include "persistence/ProfileManager.hpp"
#include "persistence/SessionHistory.hpp"
#include "persistence/SettingsManager.hpp"
#include "app/SoundSystem.hpp"
#include "stats/StatTracker.hpp"

namespace xaimassist::app {
RuntimeServices CompositionRoot::CreateRuntimeServices() const {
    RuntimeServices services;
    services.eventBus = std::make_shared<core::EventBus>();
    services.frameClock = std::make_shared<core::FrameClock>(1.0 / 144.0);
    services.logger = std::make_shared<core::Logger>();
    services.engine =
        std::make_shared<engine::Engine>(*services.eventBus, *services.logger);
    services.inputManager = std::make_shared<input::InputManager>(*services.logger);

    services.persistenceDatabase =
        std::make_shared<persistence::PersistenceDatabase>(services.logger.get());
    if (!services.persistenceDatabase->Initialize()) {
        services.logger->Error("persistence",
                               "Failed to Initialize SQLite persistence database");
    }

    services.profileManager = std::make_shared<persistence::ProfileManager>(
        *services.persistenceDatabase, *services.logger);
    if (!services.profileManager->EnsureDefaultProfile()) {
        services.logger->Error("persistence", "Failed to ensure default profile");
    }

    services.settingsManager = std::make_shared<persistence::SettingsManager>(
        *services.persistenceDatabase, *services.profileManager);
    if (!services.settingsManager->Load()) {
        services.logger->Warning(
            "persistence", "Failed to Load profile Settings from persistence");
    }

    services.statTracker = std::make_shared<stats::StatTracker>(
        *services.eventBus, *services.logger);
    services.sessionHistory = std::make_shared<persistence::SessionHistory>(
        *services.persistenceDatabase, *services.profileManager,
        *services.eventBus, *services.logger);

    services.soundSystem = std::make_shared<SoundSystem>(*services.eventBus);
    services.soundSystem->ApplySettings(
        services.settingsManager->Settings().sound);

    return services;
}
}  // namespace xaimassist::app
