/// @file AppGameplaySettingsProvider.cpp
#include "app/AppGameplaySettingsProvider.hpp"

#include "persistence/SettingsManager.hpp"

namespace xaimassist::app {
AppGameplaySettingsProvider::AppGameplaySettingsProvider(
    const persistence::SettingsManager& settingsManager)
    : m_settingsManager(settingsManager) {}

gameplay::GameplayRuntimeSettings
AppGameplaySettingsProvider::CurrentSettings() const {
    const auto& Settings = m_settingsManager.Settings();

    gameplay::GameplayRuntimeSettings runtimeSettings;
    runtimeSettings.targetColor = {Settings.gameplay.target.color.red,
                                   Settings.gameplay.target.color.green,
                                   Settings.gameplay.target.color.blue};
    runtimeSettings.TargetRadius = Settings.gameplay.target.radius;
    return runtimeSettings;
}
}  // namespace xaimassist::app
