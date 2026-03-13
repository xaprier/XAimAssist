/// @file SettingsConverter.cpp
#include "app/SettingsConverter.hpp"

#include <algorithm>

namespace xaimassist::app::SettingsConverter {

input::SensitivitySettings ToSensitivitySettings(const persistence::AppSettings& settings) {
    input::SensitivitySettings sensitivitySettings;
    sensitivitySettings.cmPer360 = settings.input.sensitivity.cmPer360;
    sensitivitySettings.dpi = settings.input.sensitivity.dpi;
    sensitivitySettings.sensitivityScale =
        settings.input.sensitivity.sensitivityScale;
    sensitivitySettings.yawMultiplier = settings.input.sensitivity.yawMultiplier;
    sensitivitySettings.pitchMultiplier =
        settings.input.sensitivity.pitchMultiplier;
    sensitivitySettings.invertY = settings.input.sensitivity.invertY;
    return sensitivitySettings;
}

std::vector<ui::UiViewModel::ModeDescriptor> ToModeDescriptors(std::vector<gameplay::GameModeMetadata> metadata) {
    // Sort alphabetically by display name
    std::sort(metadata.begin(), metadata.end(),
              [](const gameplay::GameModeMetadata& left,
                 const gameplay::GameModeMetadata& right) {
                  return left.displayName < right.displayName;
              });

    std::vector<ui::UiViewModel::ModeDescriptor> descriptors;
    descriptors.reserve(metadata.size());

    for (const auto& modeMetadata : metadata) {
        std::vector<ui::UiViewModel::ModeSettingDescriptor> settingDescriptors;
        settingDescriptors.reserve(modeMetadata.settings.size());

        for (const auto& setting : modeMetadata.settings) {
            settingDescriptors.push_back(
                ui::UiViewModel::ModeSettingDescriptor{
                    setting.key,
                    setting.displayName,
                    setting.defaultValue,
                    setting.minValue,
                    setting.maxValue,
                    setting.step,
                    setting.integerOnly});
        }

        descriptors.push_back(ui::UiViewModel::ModeDescriptor{
            modeMetadata.id,
            modeMetadata.displayName,
            modeMetadata.description,
            modeMetadata.defaultDurationSeconds,
            modeMetadata.defaultDistanceUnits,
            modeMetadata.defaultGridRows,
            modeMetadata.defaultGridColumns,
            modeMetadata.defaultActiveTargetCount,
            settingDescriptors});
    }

    return descriptors;
}

std::string ThemeModeToString(persistence::UiThemeMode themeMode) {
    switch (themeMode) {
        case persistence::UiThemeMode::Light:
            return "light";
        case persistence::UiThemeMode::Dark:
        default:
            return "dark";
    }
}

std::string LanguageToString(persistence::UiLanguage language) {
    switch (language) {
        case persistence::UiLanguage::Turkish:
            return "tr";
        case persistence::UiLanguage::English:
        default:
            return "en";
    }
}

}  // namespace xaimassist::app::SettingsConverter
