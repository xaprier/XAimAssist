/**
 * @file SettingsManager.hpp
 * @brief Load/save AppSettings from the profile database
 *        with legacy INI import support.
 */

#ifndef SETTINGSMANAGER_HPP
#define SETTINGSMANAGER_HPP

#include <cstdint>
#include <string>

#include "persistence/AppSettings.hpp"

namespace xaimassist::persistence {
class PersistenceDatabase;
class ProfileManager;

/**
 * @class SettingsManager
 * @brief Reads/writes serialised AppSettings from/to the SQLite database,
 *        optionally importing data from a legacy QSettings INI file.
 */
class SettingsManager {
  public:
    SettingsManager(
        PersistenceDatabase& database, ProfileManager& profileManager,
        std::string legacySettingsFilePath = DefaultLegacySettingsFilePath());

    /// Load settings from the database (with optional legacy INI import).
    bool Load();

    /// Persist current settings to the database.
    bool Save() const;

    /// Read-only access to the in-memory settings.
    const AppSettings& Settings() const noexcept;

    /// Replace the in-memory settings.
    void SetSettings(const AppSettings& Settings);

    /// Return the application-wide default settings.
    static AppSettings DefaultSettings();

    /// Platform-default path for the legacy QSettings INI file.
    static std::string DefaultLegacySettingsFilePath();

  private:
    bool _LoadFromDatabase(std::int64_t profileId);
    bool _SaveToDatabase(std::int64_t profileId) const;
    bool _ImportLegacyIni(std::int64_t profileId);

    static UiThemeMode _ParseThemeMode(const std::string& value) noexcept;
    static UiLanguage _ParseLanguage(const std::string& value) noexcept;
    static UiOverlayAnchor _ParseOverlayAnchor(const std::string& value) noexcept;
    static HitSoundVariant _ParseHitSoundVariant(const std::string& value) noexcept;
    static MissSoundVariant _ParseMissSoundVariant(const std::string& value) noexcept;
    static std::string _ToStorageValue(UiThemeMode ThemeMode);
    static std::string _ToStorageValue(UiLanguage language);
    static std::string _ToStorageValue(UiOverlayAnchor anchor);
    static std::string _ToStorageValue(HitSoundVariant variant);
    static std::string _ToStorageValue(MissSoundVariant variant);

    static double _ClampMin(double value, double minimum) noexcept;
    static double _Clamp01(double value) noexcept;

    PersistenceDatabase& m_database;
    ProfileManager& m_profileManager;
    std::string m_legacySettingsFilePath;
    AppSettings m_settings;
};
}  // namespace xaimassist::persistence

#endif  // SETTINGSMANAGER_HPP
