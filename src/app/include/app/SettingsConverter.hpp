/**
 * @file SettingsConverter.hpp
 * @brief Converts persistence layer settings to domain model types.
 */

#ifndef SETTINGS_CONVERTER_HPP
#define SETTINGS_CONVERTER_HPP

#include <string>
#include <vector>

#include "gameplay/GameModeRegistry.hpp"
#include "input/SensitivityModel.hpp"
#include "persistence/AppSettings.hpp"
#include "ui/UiViewModel.hpp"

namespace xaimassist::app {

/**
 * @namespace SettingsConverter
 * @brief Pure functions for converting settings between layers.
 *
 * These converters act as adapters between persistence and domain models,
 * maintaining separation of concerns.
 */
namespace SettingsConverter {

/// Convert persistence input settings to runtime sensitivity model.
input::SensitivitySettings ToSensitivitySettings(
    const persistence::AppSettings& settings);

/// Convert gameplay mode metadata to UI view model descriptors.
std::vector<ui::UiViewModel::ModeDescriptor> ToModeDescriptors(std::vector<gameplay::GameModeMetadata> metadata);

/// Convert UI theme enum to string for QML.
std::string ThemeModeToString(persistence::UiThemeMode themeMode);

/// Convert UI language enum to ISO code string for QML.
std::string LanguageToString(persistence::UiLanguage language);

}  // namespace SettingsConverter
}  // namespace xaimassist::app

#endif  // SETTINGS_CONVERTER_HPP
