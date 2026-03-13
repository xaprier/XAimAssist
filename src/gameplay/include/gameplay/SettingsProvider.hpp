/**
 * @file SettingsProvider.hpp
 * @brief Runtime gameplay settings interface.
 */

#ifndef SETTINGSPROVIDER_HPP
#define SETTINGSPROVIDER_HPP

#include <array>

namespace xaimassist::gameplay {

/// Current target appearance settings read by game modes at spawn time.
struct GameplayRuntimeSettings {
    std::array<double, 3> targetColor{0.95, 0.35, 0.25};
    double TargetRadius{0.5};
};

/**
 * @class IGameplaySettingsProvider
 * @brief Provides live gameplay preferences (target colour, radius, etc.).
 */
class IGameplaySettingsProvider {
  public:
    virtual ~IGameplaySettingsProvider() = default;

    /// Return the current target appearance preferences.
    virtual GameplayRuntimeSettings CurrentSettings() const = 0;
};
}  // namespace xaimassist::gameplay

#endif  // SETTINGSPROVIDER_HPP
