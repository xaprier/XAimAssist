/**
 * @file CompositionRoot.hpp
 * @brief Dependency injection root — creates and wires all runtime services.
 */

#ifndef COMPOSITIONROOT_HPP
#define COMPOSITIONROOT_HPP

#include <memory>

namespace xaimassist::core {
class EventBus;
class FrameClock;
class Logger;
}  // namespace xaimassist::core

namespace xaimassist::engine {
class Engine;
}

namespace xaimassist::input {
class InputManager;
}

namespace xaimassist::persistence {
class PersistenceDatabase;
class ProfileManager;
class SessionHistory;
class SettingsManager;
}  // namespace xaimassist::persistence

namespace xaimassist::stats {
class StatTracker;
}

namespace xaimassist::app {

/// Aggregates shared pointers to every core runtime service.
struct RuntimeServices {
    std::shared_ptr<core::EventBus> eventBus;
    std::shared_ptr<core::FrameClock> frameClock;
    std::shared_ptr<core::Logger> logger;
    std::shared_ptr<engine::Engine> engine;
    std::shared_ptr<input::InputManager> inputManager;
    std::shared_ptr<persistence::PersistenceDatabase> persistenceDatabase;
    std::shared_ptr<persistence::ProfileManager> profileManager;
    std::shared_ptr<persistence::SessionHistory> sessionHistory;
    std::shared_ptr<persistence::SettingsManager> settingsManager;
    std::shared_ptr<stats::StatTracker> statTracker;
};

/**
 * @class CompositionRoot
 * @brief Factory that instantiates and connects all runtime services.
 */
class CompositionRoot {
  public:
    /// Instantiate and wire every runtime service; returns them as a bundle.
    RuntimeServices CreateRuntimeServices() const;
};
}  // namespace xaimassist::app

#endif  // COMPOSITIONROOT_HPP
