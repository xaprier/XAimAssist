/**
 * @file I18nProvider.hpp
 * @brief Static translation table builder for supported UI languages.
 *
 * Centralises all user-visible string literals so that adding a new locale
 * requires changes to exactly one translation unit (I18nProvider.cpp) and
 * nowhere else.  UiViewModel delegates to this class instead of hosting the
 * string tables directly.
 */

#ifndef I18NPROVIDER_HPP
#define I18NPROVIDER_HPP

#include <QVariantMap>
#include <string>

#include "persistence/AppSettings.hpp"

namespace xaimassist::ui {

/**
 * @class I18nProvider
 * @brief Builds QVariantMap translation tables for a given UiLanguage.
 *
 * All methods are static; no instances are needed.
 */
class I18nProvider {
  public:
    I18nProvider() = delete;

    /// Return the full i18n string table for the given language.
    [[nodiscard]] static QVariantMap Build(persistence::UiLanguage language);

    /// Localised display name for a training mode, falling back to the
    /// engine-provided name if no translation exists for the given language.
    [[nodiscard]] static QString ModeName(persistence::UiLanguage language,
                                          const std::string& modeId,
                                          const std::string& fallback);

    /// Localised description for a training mode.
    [[nodiscard]] static QString ModeDescription(persistence::UiLanguage language,
                                                  const std::string& modeId,
                                                  const std::string& fallback);

    /// Localised display name for a mode-specific setting.
    [[nodiscard]] static QString ModeSettingName(persistence::UiLanguage language,
                                                  const std::string& modeId,
                                                  const std::string& settingKey,
                                                  const std::string& fallback);

  private:
    [[nodiscard]] static QVariantMap _BuildEnglish();
    [[nodiscard]] static QVariantMap _BuildTurkish();
};

}  // namespace xaimassist::ui

#endif  // I18NPROVIDER_HPP
