/// @file GameMode.cpp
#include "gameplay/GameMode.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace xaimassist::gameplay {
GameMode::GameMode(GameModeMetadata Metadata, core::EventBus& eventBus,
                   core::Logger& logger, ISceneCommandSink& sceneCommandSink,
                   TargetSystem& targetSystem,
                   IGameplaySettingsProvider& settingsProvider)
    : m_metadata(std::move(Metadata)), m_eventBus(eventBus), m_logger(logger), m_sceneCommandSink(sceneCommandSink), m_targetSystem(targetSystem), m_settingsProvider(settingsProvider), m_configuredDistanceUnits(m_metadata.defaultDistanceUnits), m_configuredGridRows(m_metadata.defaultGridRows), m_configuredGridColumns(m_metadata.defaultGridColumns), m_configuredActiveTargetCount(m_metadata.defaultActiveTargetCount) {}

const GameModeMetadata& GameMode::Metadata() const noexcept {
    return m_metadata;
}

void GameMode::SetSessionId(std::uint64_t SessionId) noexcept {
    m_sessionId = SessionId;
}

void GameMode::SetConfiguredDistance(double distanceUnits) noexcept {
    m_configuredDistanceUnits = distanceUnits;
}

void GameMode::SetConfiguredGridLayout(int rows, int columns,
                                       int activeTargetCount) noexcept {
    m_configuredGridRows = rows;
    m_configuredGridColumns = columns;
    m_configuredActiveTargetCount = activeTargetCount;
}

void GameMode::ApplyModeSettings(
    const std::unordered_map<std::string, double>& requestedSettings) {
    m_modeSettings.clear();

    for (const auto& setting : m_metadata.settings) {
        const auto requestedIterator = requestedSettings.find(setting.key);
        const double requestedValue = requestedIterator != requestedSettings.end()
                                          ? requestedIterator->second
                                          : setting.defaultValue;

        const double low = std::min(setting.minValue, setting.maxValue);
        const double high = std::max(setting.minValue, setting.maxValue);

        double clampedValue = std::clamp(requestedValue, low, high);
        if (setting.integerOnly) {
            clampedValue = std::round(clampedValue);
        }

        m_modeSettings.emplace(setting.key, clampedValue);
    }
}

core::EventBus& GameMode::GetEventBus() noexcept { return m_eventBus; }

core::Logger& GameMode::GetLogger() noexcept { return m_logger; }

ISceneCommandSink& GameMode::GetSceneCommandSink() noexcept {
    return m_sceneCommandSink;
}

TargetSystem& GameMode::GetTargetSystem() noexcept { return m_targetSystem; }

IGameplaySettingsProvider& GameMode::GetSettingsProvider() noexcept {
    return m_settingsProvider;
}

std::uint64_t GameMode::SessionId() const noexcept { return m_sessionId; }

double GameMode::ConfiguredDistance() const noexcept {
    return m_configuredDistanceUnits;
}

int GameMode::ConfiguredGridRows() const noexcept {
    return m_configuredGridRows;
}

int GameMode::ConfiguredGridColumns() const noexcept {
    return m_configuredGridColumns;
}

int GameMode::ConfiguredActiveTargetCount() const noexcept {
    return m_configuredActiveTargetCount;
}

double GameMode::ModeSettingValue(const std::string& key,
                                  double fallback) const noexcept {
    const auto iterator = m_modeSettings.find(key);
    if (iterator == m_modeSettings.end()) {
        return fallback;
    }

    return iterator->second;
}

const std::unordered_map<std::string, double>&
GameMode::ModeSettings() const noexcept {
    return m_modeSettings;
}
}  // namespace xaimassist::gameplay
