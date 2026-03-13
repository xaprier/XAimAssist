/**
 * @file GameMode.hpp
 * @brief Base class and metadata types for all training game modes.
 */

#ifndef GAMEMODE_HPP
#define GAMEMODE_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "gameplay/SceneCommandSink.hpp"
#include "gameplay/SettingsProvider.hpp"

namespace xaimassist::core {
class EventBus;
class Logger;
}  // namespace xaimassist::core

namespace xaimassist::gameplay {
class TargetSystem;

/// Descriptor for a single numeric setting exposed by a game mode.
struct GameModeSettingMetadata {
    std::string key;
    std::string displayName;
    double defaultValue{0.0};
    double minValue{0.0};
    double maxValue{0.0};
    double step{1.0};
    bool integerOnly{false};
};

/// Static descriptor published by each mode for UI and orchestration.
struct GameModeMetadata {
    std::string id;
    std::string displayName;
    std::string description;
    double defaultDurationSeconds{60.0};
    double defaultDistanceUnits{25.0};
    int defaultGridRows{0};
    int defaultGridColumns{0};
    int defaultActiveTargetCount{0};
    std::vector<GameModeSettingMetadata> settings;
};

/**
 * @class GameMode
 * @brief Abstract base for training modes.
 *
 * Subclasses implement OnStart / OnUpdate / OnStop to define
 * gameplay rules, target spawning and session flow.
 */
class GameMode {
  public:
    GameMode(GameModeMetadata Metadata, core::EventBus& eventBus,
             core::Logger& logger, ISceneCommandSink& sceneCommandSink,
             TargetSystem& targetSystem,
             IGameplaySettingsProvider& settingsProvider);
    virtual ~GameMode() = default;

    /// Static descriptor for this mode (id, display name, defaults).
    const GameModeMetadata& Metadata() const noexcept;

    /// Assign the runtime session id before starting the mode.
    void SetSessionId(std::uint64_t SessionId) noexcept;

    /// Configure the target distance from the camera.
    void SetConfiguredDistance(double distanceUnits) noexcept;

    /// Configure grid layout dimensions and active target count.
    void SetConfiguredGridLayout(int rows, int columns,
                                 int activeTargetCount) noexcept;

    /// Apply mode-specific numeric settings; subclasses may override.
    virtual void ApplyModeSettings(
        const std::unordered_map<std::string, double>& requestedSettings);

    /// Called once when the session begins.
    virtual void OnStart() = 0;

    /// Called every frame with the delta time in seconds.
    virtual void OnUpdate(double dtSeconds) = 0;

    /// Called when the session ends.
    virtual void OnStop() = 0;

  protected:
    /// Access the shared event bus.
    core::EventBus& GetEventBus() noexcept;

    /// Access the shared logger.
    core::Logger& GetLogger() noexcept;

    /// Access the scene command sink for spawn/update/destroy.
    ISceneCommandSink& GetSceneCommandSink() noexcept;

    /// Access the target system for spawning and querying targets.
    TargetSystem& GetTargetSystem() noexcept;

    /// Access the runtime gameplay settings provider.
    IGameplaySettingsProvider& GetSettingsProvider() noexcept;

    /// Current session id.
    std::uint64_t SessionId() const noexcept;

    /// Configured target distance from camera in world units.
    double ConfiguredDistance() const noexcept;

    /// Configured grid row count (0 if grid not applicable).
    int ConfiguredGridRows() const noexcept;

    /// Configured grid column count (0 if grid not applicable).
    int ConfiguredGridColumns() const noexcept;

    /// Configured number of simultaneously active targets.
    int ConfiguredActiveTargetCount() const noexcept;

    /// Retrieve a mode-specific setting value, or fallback if absent.
    double ModeSettingValue(const std::string& key,
                            double fallback = 0.0) const noexcept;

    /// All mode-specific setting values.
    const std::unordered_map<std::string, double>& ModeSettings() const noexcept;

  private:
    GameModeMetadata m_metadata;
    core::EventBus& m_eventBus;
    core::Logger& m_logger;
    ISceneCommandSink& m_sceneCommandSink;
    TargetSystem& m_targetSystem;
    IGameplaySettingsProvider& m_settingsProvider;
    std::uint64_t m_sessionId{0};
    double m_configuredDistanceUnits{25.0};
    int m_configuredGridRows{0};
    int m_configuredGridColumns{0};
    int m_configuredActiveTargetCount{0};
    std::unordered_map<std::string, double> m_modeSettings;
};
}  // namespace xaimassist::gameplay

#endif  // GAMEMODE_HPP
