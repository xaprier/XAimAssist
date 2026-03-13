/**
 * @file AppGameplaySettingsProvider.hpp
 * @brief Bridges SettingsManager to IGameplaySettingsProvider.
 */

#ifndef APPGAMEPLAYSETTINGSPROVIDER_HPP
#define APPGAMEPLAYSETTINGSPROVIDER_HPP

#include "gameplay/SettingsProvider.hpp"

namespace xaimassist::persistence {
class SettingsManager;
}

namespace xaimassist::app {

/**
 * @class AppGameplaySettingsProvider
 * @brief Reads live target colour/radius from the persisted settings.
 */
class AppGameplaySettingsProvider final
    : public gameplay::IGameplaySettingsProvider {
  public:
    explicit AppGameplaySettingsProvider(
        const persistence::SettingsManager& settingsManager);

    /// Return live gameplay settings from the persisted configuration.
    gameplay::GameplayRuntimeSettings CurrentSettings() const override;

  private:
    const persistence::SettingsManager& m_settingsManager;
};
}  // namespace xaimassist::app

#endif  // APPGAMEPLAYSETTINGSPROVIDER_HPP
