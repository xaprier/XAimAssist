/**
 * @file GameModeRegistry.hpp
 * @brief Factory registry for available game modes.
 */

#ifndef GAMEMODEREGISTRY_HPP
#define GAMEMODEREGISTRY_HPP

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "gameplay/GameMode.hpp"

namespace xaimassist::core {
class EventBus;
class Logger;
}  // namespace xaimassist::core

namespace xaimassist::gameplay {
class TargetSystem;
class IGameplaySettingsProvider;

/**
 * @class GameModeRegistry
 * @brief Stores metadata + factory lambdas keyed by mode id.
 *
 * New modes are added via RegisterMode(); the orchestrator uses
 * CreateMode() to instantiate the selected mode at session start.
 */
class GameModeRegistry {
  public:
    using GameModeFactory = std::function<std::unique_ptr<GameMode>(
        core::EventBus&, core::Logger&, ISceneCommandSink&, TargetSystem&,
        IGameplaySettingsProvider&)>;

    /// Register a mode factory keyed by its metadata id. Returns false on
    /// duplicate.
    bool RegisterMode(const GameModeMetadata& Metadata, GameModeFactory factory);

    /// True if a mode with the given id has been registered.
    bool ContainsMode(const std::string& modeId) const;

    /// Return metadata for every registered mode.
    std::vector<GameModeMetadata> AvailableModes() const;

    /// Instantiate the mode identified by modeId, or nullptr if unknown.
    std::unique_ptr<GameMode> CreateMode(const std::string& modeId, core::EventBus& eventBus,
                                         core::Logger& logger, ISceneCommandSink& sceneCommandSink,
                                         TargetSystem& targetSystem,
                                         IGameplaySettingsProvider& settingsProvider) const;

  private:
    struct RegistryEntry {
        GameModeMetadata Metadata;
        GameModeFactory factory;
    };

    std::unordered_map<std::string, RegistryEntry> m_registry;
};
}  // namespace xaimassist::gameplay

#endif  // GAMEMODEREGISTRY_HPP
