/**
 * @file ModeOrchestrator.hpp
 * @brief Session lifecycle manager for game modes.
 */

#ifndef MODEORCHESTRATOR_HPP
#define MODEORCHESTRATOR_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "core/CoreEvents.hpp"
#include "gameplay/GameMode.hpp"
#include "gameplay/GameModeRegistry.hpp"

namespace xaimassist::core {
class EventBus;
class Logger;
}  // namespace xaimassist::core

namespace xaimassist::gameplay {
class TargetSystem;
class IGameplaySettingsProvider;

/**
 * @class ModeOrchestrator
 * @brief Creates, starts, ticks, and stops game mode sessions.
 *
 * Owns the active GameMode instance and drives it through its
 * lifecycle while publishing session-level events.
 */
class ModeOrchestrator {
  public:
    ModeOrchestrator(core::EventBus& eventBus, core::Logger& logger,
                     GameModeRegistry& registry,
                     ISceneCommandSink& sceneCommandSink,
                     TargetSystem& targetSystem,
                     IGameplaySettingsProvider& settingsProvider);

    /**
     * @brief Create and start a game mode session.
     * @param modeId Registered mode identifier.
     * @param durationOverrideSeconds Custom duration (0 = mode default).
     * @param distanceOverrideUnits Custom target distance (0 = mode default).
     * @param modeSettingOverrides Per-setting numeric overrides.
     * @return true if the mode was started successfully.
     */
    bool StartMode(
        const std::string& modeId, double durationOverrideSeconds = 0.0,
        double distanceOverrideUnits = 0.0,
        const std::unordered_map<std::string, double>& modeSettingOverrides = {});

    /// Tick the active mode with the given delta time.
    void Update(double dtSeconds);

    /// End the active mode with the specified reason.
    void StopMode(core::events::SessionStopReason stopReason =
                      core::events::SessionStopReason::Completed);

    /// True if a mode is currently running.
    bool HasActiveMode() const noexcept;

    /// Registered id of the currently active mode.
    const std::string& ActiveModeId() const noexcept;

    /// Runtime session id for the active session.
    std::uint64_t ActiveSessionId() const noexcept;

    /// Wall-clock seconds elapsed since the current session started.
    double ElapsedSeconds() const noexcept;

    /// Seconds remaining before the session timer expires.
    double RemainingSeconds() const noexcept;

  private:
    core::EventBus& m_eventBus;
    core::Logger& m_logger;
    GameModeRegistry& m_registry;
    ISceneCommandSink& m_sceneCommandSink;
    TargetSystem& m_targetSystem;
    IGameplaySettingsProvider& m_settingsProvider;

    std::unique_ptr<GameMode> m_activeMode;
    std::string m_activeModeId;

    std::uint64_t m_nextSessionId{1};
    std::uint64_t m_activeSessionId{0};

    double m_elapsedSeconds{0.0};
    double m_modeDurationSeconds{0.0};
};
}  // namespace xaimassist::gameplay

#endif  // MODEORCHESTRATOR_HPP
