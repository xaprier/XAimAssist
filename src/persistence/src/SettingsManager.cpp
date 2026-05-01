/// @file SettingsManager.cpp
#include "persistence/SettingsManager.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>

#include "persistence/PersistenceDatabase.hpp"
#include "persistence/ProfileManager.hpp"

namespace {
QString serializeModeOverridesJson(
    const std::unordered_map<
        std::string, xaimassist::persistence::ModePreferences>& modeOverrides) {
    QJsonObject root;

    for (const auto& [modeId, modePreferences] : modeOverrides) {
        if (modeId.empty()) {
            continue;
        }

        QJsonObject modeObject;

        if (std::isfinite(modePreferences.durationSeconds) &&
            modePreferences.durationSeconds > 0.0) {
            modeObject.insert(QStringLiteral("durationSeconds"),
                              modePreferences.durationSeconds);
        }

        if (std::isfinite(modePreferences.distanceUnits) &&
            modePreferences.distanceUnits > 0.0) {
            modeObject.insert(QStringLiteral("distanceUnits"),
                              modePreferences.distanceUnits);
        }

        QJsonObject settingValuesObject;
        for (const auto& [settingKey, settingValue] :
             modePreferences.settingValues) {
            if (settingKey.empty() || !std::isfinite(settingValue)) {
                continue;
            }

            settingValuesObject.insert(QString::fromStdString(settingKey),
                                       settingValue);
        }

        modeObject.insert(QStringLiteral("settingValues"), settingValuesObject);
        root.insert(QString::fromStdString(modeId), modeObject);
    }

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

std::unordered_map<std::string, xaimassist::persistence::ModePreferences>
parseModeOverridesJson(const QString& jsonValue) {
    std::unordered_map<std::string, xaimassist::persistence::ModePreferences>
        modeOverrides;

    const QByteArray payload = jsonValue.trimmed().toUtf8();
    if (payload.isEmpty()) {
        return modeOverrides;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return modeOverrides;
    }

    const QJsonObject root = document.object();
    for (auto modeIterator = root.begin(); modeIterator != root.end();
         ++modeIterator) {
        if (!modeIterator.value().isObject()) {
            continue;
        }

        const std::string modeId = modeIterator.key().trimmed().toStdString();
        if (modeId.empty()) {
            continue;
        }

        xaimassist::persistence::ModePreferences modePreferences;
        const QJsonObject modeObject = modeIterator.value().toObject();

        const double durationSeconds =
            modeObject.value(QStringLiteral("durationSeconds")).toDouble(0.0);
        if (std::isfinite(durationSeconds) && durationSeconds > 0.0) {
            modePreferences.durationSeconds = durationSeconds;
        }

        const double distanceUnits =
            modeObject.value(QStringLiteral("distanceUnits")).toDouble(0.0);
        if (std::isfinite(distanceUnits) && distanceUnits > 0.0) {
            modePreferences.distanceUnits = distanceUnits;
        }

        const QJsonValue settingValuesValue =
            modeObject.value(QStringLiteral("settingValues"));
        if (settingValuesValue.isObject()) {
            const QJsonObject settingValuesObject = settingValuesValue.toObject();
            for (auto settingIterator = settingValuesObject.begin();
                 settingIterator != settingValuesObject.end(); ++settingIterator) {
                if (!settingIterator.value().isDouble()) {
                    continue;
                }

                const std::string settingKey =
                    settingIterator.key().trimmed().toStdString();
                const double settingValue = settingIterator.value().toDouble();
                if (settingKey.empty() || !std::isfinite(settingValue)) {
                    continue;
                }

                modePreferences.settingValues.emplace(settingKey, settingValue);
            }
        }

        modeOverrides.emplace(modeId, std::move(modePreferences));
    }

    return modeOverrides;
}
}  // namespace

namespace xaimassist::persistence {
SettingsManager::SettingsManager(PersistenceDatabase& database,
                                 ProfileManager& profileManager,
                                 std::string legacySettingsFilePath)
    : m_database(database), m_profileManager(profileManager), m_legacySettingsFilePath(std::move(legacySettingsFilePath)), m_settings(DefaultSettings()) {}

bool SettingsManager::Load() {
    m_settings = DefaultSettings();

    const std::int64_t profileId = m_profileManager.ActiveProfileId();
    if (profileId <= 0) {
        return false;
    }

    if (_LoadFromDatabase(profileId)) {
        return true;
    }

    if (_ImportLegacyIni(profileId)) {
        return _SaveToDatabase(profileId);
    }

    return _SaveToDatabase(profileId);
}

bool SettingsManager::Save() const {
    const std::int64_t profileId = m_profileManager.ActiveProfileId();
    if (profileId <= 0) {
        return false;
    }

    return _SaveToDatabase(profileId);
}

const AppSettings& SettingsManager::Settings() const noexcept {
    return m_settings;
}

void SettingsManager::SetSettings(const AppSettings& Settings) {
    m_settings = Settings;
}

AppSettings SettingsManager::DefaultSettings() { return AppSettings{}; }

std::string SettingsManager::DefaultLegacySettingsFilePath() {
    const char* home = std::getenv("HOME");
    const std::string homePath =
        home != nullptr ? std::string(home) : std::string(".");
    return homePath + "/.config/XAimAssist/Settings.ini";
}

bool SettingsManager::_LoadFromDatabase(std::int64_t profileId) {
    QSqlDatabase database = m_database.Connection();
    if (!database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    query.prepare(
        "SELECT raw_input_enabled, cm_per_360, Dpi, sensitivity_scale, "
        "yaw_multiplier, pitch_multiplier, scoped_multiplier, "
        "invert_y, target_color_r, target_color_g, target_color_b, "
        "target_radius, theme_mode, language, "
        "crosshair_thickness, crosshair_center_dot_enabled, "
        "crosshair_center_dot_size, crosshair_color_r, "
        "crosshair_color_g, crosshair_color_b, "
        "crosshair_border_enabled, crosshair_lines_enabled, "
        "crosshair_border_thickness, "
        "crosshair_horizontal_length, crosshair_vertical_length, "
        "crosshair_gap, fps_enabled, fps_position, selected_mode_id, "
        "mode_overrides_json, "
        "window_start_fullscreen, keybind_toggle_fullscreen, "
        "keybind_toggle_fps_counter, keybind_toggle_crosshair, "
        "sound_enabled, sound_volume, sound_hit_variant, sound_miss_variant "
        "FROM profile_settings "
        "WHERE profile_id = :profileId "
        "LIMIT 1;");
    query.bindValue(":profileId", static_cast<qlonglong>(profileId));

    if (!query.exec() || !query.next()) {
        return false;
    }

    m_settings.input.rawInputEnabled = query.value(0).toInt() != 0;
    m_settings.input.sensitivity.cmPer360 =
        _ClampMin(query.value(1).toDouble(), 0.1);
    m_settings.input.sensitivity.dpi = _ClampMin(query.value(2).toDouble(), 1.0);
    m_settings.input.sensitivity.sensitivityScale =
        _ClampMin(query.value(3).toDouble(), 0.01);
    m_settings.input.sensitivity.yawMultiplier =
        _ClampMin(query.value(4).toDouble(), 0.01);
    m_settings.input.sensitivity.pitchMultiplier =
        _ClampMin(query.value(5).toDouble(), 0.01);
    m_settings.input.sensitivity.scopedMultiplier =
        _ClampMin(query.value(6).toDouble(), 0.01);
    m_settings.input.sensitivity.invertY = query.value(7).toInt() != 0;

    m_settings.gameplay.target.color.red = _Clamp01(query.value(8).toDouble());
    m_settings.gameplay.target.color.green = _Clamp01(query.value(9).toDouble());
    m_settings.gameplay.target.color.blue = _Clamp01(query.value(10).toDouble());
    m_settings.gameplay.target.radius =
        _ClampMin(query.value(11).toDouble(), 0.05);

    m_settings.ui.themeMode =
        _ParseThemeMode(query.value(12).toString().toStdString());
    m_settings.ui.language =
        _ParseLanguage(query.value(13).toString().toStdString());

    m_settings.ui.crosshair.thickness =
        std::clamp(query.value(14).toDouble(), 1.0, 12.0);
    m_settings.ui.crosshair.centerDotEnabled = query.value(15).toInt() != 0;
    m_settings.ui.crosshair.centerDotSize =
        std::clamp(query.value(16).toDouble(), 1.0, 16.0);
    m_settings.ui.crosshair.color.red = _Clamp01(query.value(17).toDouble());
    m_settings.ui.crosshair.color.green = _Clamp01(query.value(18).toDouble());
    m_settings.ui.crosshair.color.blue = _Clamp01(query.value(19).toDouble());
    m_settings.ui.crosshair.borderEnabled = query.value(20).toInt() != 0;
    m_settings.ui.crosshair.linesEnabled = query.value(21).toInt() != 0;
    m_settings.ui.crosshair.borderThickness =
        std::clamp(query.value(22).toDouble(), 1.0, 6.0);
    m_settings.ui.crosshair.horizontalLength =
        std::clamp(query.value(23).toDouble(), 2.0, 40.0);
    m_settings.ui.crosshair.verticalLength =
        std::clamp(query.value(24).toDouble(), 2.0, 40.0);
    m_settings.ui.crosshair.gap =
        std::clamp(query.value(25).toDouble(), 0.0, 20.0);

    m_settings.ui.fpsCounter.enabled = query.value(26).toInt() != 0;
    m_settings.ui.fpsCounter.anchor =
        _ParseOverlayAnchor(query.value(27).toString().toStdString());
    m_settings.gameplay.selectedModeId = query.value(28).toString().toStdString();
    if (m_settings.gameplay.selectedModeId.empty()) {
        m_settings.gameplay.selectedModeId = "static_sphere";
    }

    m_settings.gameplay.modeOverrides =
        parseModeOverridesJson(query.value(29).toString());

    m_settings.window.startFullscreen = query.value(30).toInt() != 0;

    const auto loadKey = [&](int col, std::string& target) {
        const std::string v = query.value(col).toString().trimmed().toStdString();
        if (!v.empty()) {
            target = v;
        }
    };
    loadKey(31, m_settings.keybindings.toggleFullscreen);
    loadKey(32, m_settings.keybindings.toggleFpsCounter);
    loadKey(33, m_settings.keybindings.toggleCrosshair);

    m_settings.sound.enabled = query.value(34).toInt() != 0;
    m_settings.sound.volume =
        static_cast<float>(std::clamp(query.value(35).toDouble(), 0.0, 1.0));
    m_settings.sound.hitSound =
        _ParseHitSoundVariant(query.value(36).toString().toStdString());
    m_settings.sound.missSound =
        _ParseMissSoundVariant(query.value(37).toString().toStdString());

    return true;
}

bool SettingsManager::_SaveToDatabase(std::int64_t profileId) const {
    QSqlDatabase database = m_database.Connection();
    if (!database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    query.prepare(
        "INSERT INTO profile_settings ("
        "profile_id, raw_input_enabled, cm_per_360, Dpi, sensitivity_scale, "
        "yaw_multiplier, pitch_multiplier, scoped_multiplier, "
        "invert_y, target_color_r, target_color_g, target_color_b, "
        "target_radius, theme_mode, language, "
        "crosshair_thickness, crosshair_center_dot_enabled, "
        "crosshair_center_dot_size, crosshair_color_r, "
        "crosshair_color_g, crosshair_color_b, "
        "crosshair_border_enabled, crosshair_lines_enabled, "
        "crosshair_border_thickness, "
        "crosshair_horizontal_length, crosshair_vertical_length, "
        "crosshair_gap, fps_enabled, fps_position, selected_mode_id, "
        "mode_overrides_json, "
        "window_start_fullscreen, keybind_toggle_fullscreen, "
        "keybind_toggle_fps_counter, keybind_toggle_crosshair, "
        "sound_enabled, sound_volume, sound_hit_variant, sound_miss_variant, "
        "updated_at"
        ") VALUES ("
        ":profileId, :RawInputEnabled, :CmPer360, :Dpi, :SensitivityScale, "
        ":YawMultiplier, :PitchMultiplier, :scopedMultiplier, "
        ":InvertY, :targetColorR, :targetColorG, :targetColorB, "
        ":TargetRadius, :ThemeMode, :language, "
        ":CrosshairThickness, :CrosshairCenterDotEnabled, "
        ":CrosshairCenterDotSize, :crosshairColorR, "
        ":crosshairColorG, :crosshairColorB, "
        ":CrosshairBorderEnabled, :CrosshairLinesEnabled, "
        ":CrosshairBorderThickness, "
        ":CrosshairHorizontalLength, :CrosshairVerticalLength, "
        ":CrosshairGap, :fpsEnabled, :fpsPosition, :SelectedModeId, "
        ":ModeOverridesJson, "
        ":windowStartFullscreen, :keybindToggleFullscreen, "
        ":keybindToggleFpsCounter, :keybindToggleCrosshair, "
        ":soundEnabled, :soundVolume, :soundHitVariant, :soundMissVariant, "
        "datetime('now')"
        ")"
        "ON CONFLICT(profile_id) DO UPDATE SET "
        "raw_input_enabled = excluded.raw_input_enabled,"
        "cm_per_360 = excluded.cm_per_360,"
        "Dpi = excluded.Dpi,"
        "sensitivity_scale = excluded.sensitivity_scale,"
        "yaw_multiplier = excluded.yaw_multiplier,"
        "pitch_multiplier = excluded.pitch_multiplier,"
        "scoped_multiplier = excluded.scoped_multiplier,"
        "invert_y = excluded.invert_y,"
        "target_color_r = excluded.target_color_r,"
        "target_color_g = excluded.target_color_g,"
        "target_color_b = excluded.target_color_b,"
        "target_radius = excluded.target_radius,"
        "theme_mode = excluded.theme_mode,"
        "language = excluded.language,"
        "crosshair_thickness = excluded.crosshair_thickness,"
        "crosshair_center_dot_enabled = "
        "excluded.crosshair_center_dot_enabled,"
        "crosshair_center_dot_size = excluded.crosshair_center_dot_size,"
        "crosshair_color_r = excluded.crosshair_color_r,"
        "crosshair_color_g = excluded.crosshair_color_g,"
        "crosshair_color_b = excluded.crosshair_color_b,"
        "crosshair_border_enabled = excluded.crosshair_border_enabled,"
        "crosshair_lines_enabled = excluded.crosshair_lines_enabled,"
        "crosshair_border_thickness = "
        "excluded.crosshair_border_thickness,"
        "crosshair_horizontal_length = "
        "excluded.crosshair_horizontal_length,"
        "crosshair_vertical_length = excluded.crosshair_vertical_length,"
        "crosshair_gap = excluded.crosshair_gap,"
        "fps_enabled = excluded.fps_enabled,"
        "fps_position = excluded.fps_position,"
        "selected_mode_id = excluded.selected_mode_id,"
        "mode_overrides_json = excluded.mode_overrides_json,"
        "window_start_fullscreen = excluded.window_start_fullscreen,"
        "keybind_toggle_fullscreen = excluded.keybind_toggle_fullscreen,"
        "keybind_toggle_fps_counter = excluded.keybind_toggle_fps_counter,"
        "keybind_toggle_crosshair = excluded.keybind_toggle_crosshair,"
        "sound_enabled = excluded.sound_enabled,"
        "sound_volume = excluded.sound_volume,"
        "sound_hit_variant = excluded.sound_hit_variant,"
        "sound_miss_variant = excluded.sound_miss_variant,"
        "updated_at = datetime('now');");

    query.bindValue(":profileId", static_cast<qlonglong>(profileId));
    query.bindValue(":RawInputEnabled", m_settings.input.rawInputEnabled ? 1 : 0);
    query.bindValue(":CmPer360", m_settings.input.sensitivity.cmPer360);
    query.bindValue(":Dpi", m_settings.input.sensitivity.dpi);
    query.bindValue(":SensitivityScale",
                    m_settings.input.sensitivity.sensitivityScale);
    query.bindValue(":YawMultiplier", m_settings.input.sensitivity.yawMultiplier);
    query.bindValue(":PitchMultiplier",
                    m_settings.input.sensitivity.pitchMultiplier);
    query.bindValue(":scopedMultiplier",
                    m_settings.input.sensitivity.scopedMultiplier);
    query.bindValue(":InvertY", m_settings.input.sensitivity.invertY ? 1 : 0);
    query.bindValue(":targetColorR", m_settings.gameplay.target.color.red);
    query.bindValue(":targetColorG", m_settings.gameplay.target.color.green);
    query.bindValue(":targetColorB", m_settings.gameplay.target.color.blue);
    query.bindValue(":TargetRadius", m_settings.gameplay.target.radius);
    query.bindValue(":ThemeMode", QString::fromStdString(
                                      _ToStorageValue(m_settings.ui.themeMode)));
    query.bindValue(":language", QString::fromStdString(
                                     _ToStorageValue(m_settings.ui.language)));
    query.bindValue(":CrosshairThickness", m_settings.ui.crosshair.thickness);
    query.bindValue(":CrosshairCenterDotEnabled",
                    m_settings.ui.crosshair.centerDotEnabled ? 1 : 0);
    query.bindValue(":CrosshairCenterDotSize",
                    m_settings.ui.crosshair.centerDotSize);
    query.bindValue(":crosshairColorR", m_settings.ui.crosshair.color.red);
    query.bindValue(":crosshairColorG", m_settings.ui.crosshair.color.green);
    query.bindValue(":crosshairColorB", m_settings.ui.crosshair.color.blue);
    query.bindValue(":CrosshairBorderEnabled",
                    m_settings.ui.crosshair.borderEnabled ? 1 : 0);
    query.bindValue(":CrosshairLinesEnabled",
                    m_settings.ui.crosshair.linesEnabled ? 1 : 0);
    query.bindValue(":CrosshairBorderThickness",
                    m_settings.ui.crosshair.borderThickness);
    query.bindValue(":CrosshairHorizontalLength",
                    m_settings.ui.crosshair.horizontalLength);
    query.bindValue(":CrosshairVerticalLength",
                    m_settings.ui.crosshair.verticalLength);
    query.bindValue(":CrosshairGap", m_settings.ui.crosshair.gap);
    query.bindValue(":fpsEnabled", m_settings.ui.fpsCounter.enabled ? 1 : 0);
    query.bindValue(":fpsPosition", QString::fromStdString(_ToStorageValue(
                                        m_settings.ui.fpsCounter.anchor)));
    query.bindValue(":SelectedModeId",
                    QString::fromStdString(m_settings.gameplay.selectedModeId));
    query.bindValue(":ModeOverridesJson", serializeModeOverridesJson(
                                              m_settings.gameplay.modeOverrides));
    query.bindValue(":windowStartFullscreen",
                    m_settings.window.startFullscreen ? 1 : 0);
    query.bindValue(":keybindToggleFullscreen",
                    QString::fromStdString(m_settings.keybindings.toggleFullscreen));
    query.bindValue(":keybindToggleFpsCounter",
                    QString::fromStdString(m_settings.keybindings.toggleFpsCounter));
    query.bindValue(":keybindToggleCrosshair",
                    QString::fromStdString(m_settings.keybindings.toggleCrosshair));
    query.bindValue(":soundEnabled", m_settings.sound.enabled ? 1 : 0);
    query.bindValue(":soundVolume", static_cast<double>(m_settings.sound.volume));
    query.bindValue(":soundHitVariant",
                    QString::fromStdString(_ToStorageValue(m_settings.sound.hitSound)));
    query.bindValue(":soundMissVariant",
                    QString::fromStdString(_ToStorageValue(m_settings.sound.missSound)));

    return query.exec();
}

bool SettingsManager::_ImportLegacyIni(std::int64_t profileId) {
    (void)profileId;

    if (!std::filesystem::exists(m_legacySettingsFilePath)) {
        return false;
    }

    const QString filePath = QString::fromStdString(m_legacySettingsFilePath);
    QSettings qSettings(filePath, QSettings::IniFormat);

    qSettings.beginGroup("input");
    m_settings.input.rawInputEnabled =
        qSettings.value("RawInputEnabled", m_settings.input.rawInputEnabled)
            .toBool();

    qSettings.beginGroup("sensitivity");
    m_settings.input.sensitivity.cmPer360 = _ClampMin(
        qSettings.value("CmPer360", m_settings.input.sensitivity.cmPer360)
            .toDouble(),
        0.1);
    m_settings.input.sensitivity.dpi = _ClampMin(
        qSettings.value("Dpi", m_settings.input.sensitivity.dpi).toDouble(), 1.0);
    m_settings.input.sensitivity.sensitivityScale =
        _ClampMin(qSettings
                      .value("SensitivityScale",
                             m_settings.input.sensitivity.sensitivityScale)
                      .toDouble(),
                  0.01);
    m_settings.input.sensitivity.yawMultiplier = _ClampMin(
        qSettings
            .value("YawMultiplier", m_settings.input.sensitivity.yawMultiplier)
            .toDouble(),
        0.01);
    m_settings.input.sensitivity.pitchMultiplier =
        _ClampMin(qSettings
                      .value("PitchMultiplier",
                             m_settings.input.sensitivity.pitchMultiplier)
                      .toDouble(),
                  0.01);
    m_settings.input.sensitivity.scopedMultiplier =
        _ClampMin(qSettings
                      .value("scopedMultiplier",
                             m_settings.input.sensitivity.scopedMultiplier)
                      .toDouble(),
                  0.01);
    m_settings.input.sensitivity.invertY =
        qSettings.value("InvertY", m_settings.input.sensitivity.invertY).toBool();
    qSettings.endGroup();
    qSettings.endGroup();

    qSettings.beginGroup("gameplay");
    qSettings.beginGroup("target");
    m_settings.gameplay.target.color.red =
        _Clamp01(qSettings.value("colorRed", m_settings.gameplay.target.color.red)
                     .toDouble());
    m_settings.gameplay.target.color.green = _Clamp01(
        qSettings.value("colorGreen", m_settings.gameplay.target.color.green)
            .toDouble());
    m_settings.gameplay.target.color.blue = _Clamp01(
        qSettings.value("colorBlue", m_settings.gameplay.target.color.blue)
            .toDouble());
    m_settings.gameplay.target.radius = _ClampMin(
        qSettings.value("radius", m_settings.gameplay.target.radius).toDouble(),
        0.05);
    qSettings.endGroup();
    qSettings.endGroup();

    qSettings.beginGroup("ui");
    qSettings.beginGroup("crosshair");
    m_settings.ui.crosshair.thickness =
        std::clamp(qSettings.value("thickness", m_settings.ui.crosshair.thickness)
                       .toDouble(),
                   1.0, 12.0);
    m_settings.ui.crosshair.centerDotEnabled =
        qSettings
            .value("centerDotEnabled", m_settings.ui.crosshair.centerDotEnabled)
            .toBool();
    m_settings.ui.crosshair.centerDotSize = std::clamp(
        qSettings.value("centerDotSize", m_settings.ui.crosshair.centerDotSize)
            .toDouble(),
        1.0, 16.0);
    m_settings.ui.crosshair.color.red =
        _Clamp01(qSettings.value("colorRed", m_settings.ui.crosshair.color.red)
                     .toDouble());
    m_settings.ui.crosshair.color.green = _Clamp01(
        qSettings.value("colorGreen", m_settings.ui.crosshair.color.green)
            .toDouble());
    m_settings.ui.crosshair.color.blue =
        _Clamp01(qSettings.value("colorBlue", m_settings.ui.crosshair.color.blue)
                     .toDouble());
    m_settings.ui.crosshair.linesEnabled =
        qSettings.value("linesEnabled", m_settings.ui.crosshair.linesEnabled)
            .toBool();
    m_settings.ui.crosshair.borderEnabled =
        qSettings.value("borderEnabled", m_settings.ui.crosshair.borderEnabled)
            .toBool();
    m_settings.ui.crosshair.borderThickness = std::clamp(
        qSettings
            .value("borderThickness", m_settings.ui.crosshair.borderThickness)
            .toDouble(),
        1.0, 6.0);
    m_settings.ui.crosshair.horizontalLength = std::clamp(
        qSettings
            .value("horizontalLength", m_settings.ui.crosshair.horizontalLength)
            .toDouble(),
        2.0, 40.0);
    m_settings.ui.crosshair.verticalLength = std::clamp(
        qSettings.value("verticalLength", m_settings.ui.crosshair.verticalLength)
            .toDouble(),
        2.0, 40.0);
    m_settings.ui.crosshair.gap =
        std::clamp(qSettings.value("gap", m_settings.ui.crosshair.gap).toDouble(),
                   0.0, 20.0);
    qSettings.endGroup();

    qSettings.beginGroup("fps");
    m_settings.ui.fpsCounter.enabled =
        qSettings.value("enabled", m_settings.ui.fpsCounter.enabled).toBool();
    m_settings.ui.fpsCounter.anchor = _ParseOverlayAnchor(
        qSettings.value("position", QStringLiteral("top_right"))
            .toString()
            .toStdString());
    qSettings.endGroup();
    qSettings.endGroup();

    return qSettings.status() == QSettings::NoError;
}

double SettingsManager::_ClampMin(double value, double minimum) noexcept {
    return std::max(value, minimum);
}

double SettingsManager::_Clamp01(double value) noexcept {
    return std::clamp(value, 0.0, 1.0);
}

UiThemeMode
SettingsManager::_ParseThemeMode(const std::string& value) noexcept {
    if (value == "light") {
        return UiThemeMode::Light;
    }

    return UiThemeMode::Dark;
}

UiLanguage SettingsManager::_ParseLanguage(const std::string& value) noexcept {
    if (value == "tr") {
        return UiLanguage::Turkish;
    }

    return UiLanguage::English;
}

UiOverlayAnchor
SettingsManager::_ParseOverlayAnchor(const std::string& value) noexcept {
    if (value == "top_left") {
        return UiOverlayAnchor::TopLeft;
    }

    if (value == "bottom_left") {
        return UiOverlayAnchor::BottomLeft;
    }

    if (value == "bottom_right") {
        return UiOverlayAnchor::BottomRight;
    }

    return UiOverlayAnchor::TopRight;
}

std::string SettingsManager::_ToStorageValue(UiThemeMode ThemeMode) {
    switch (ThemeMode) {
        case UiThemeMode::Light:
            return "light";
        case UiThemeMode::Dark:
        default:
            return "dark";
    }
}

std::string SettingsManager::_ToStorageValue(UiLanguage language) {
    switch (language) {
        case UiLanguage::Turkish:
            return "tr";
        case UiLanguage::English:
        default:
            return "en";
    }
}

std::string SettingsManager::_ToStorageValue(UiOverlayAnchor anchor) {
    switch (anchor) {
        case UiOverlayAnchor::TopLeft:
            return "top_left";
        case UiOverlayAnchor::BottomLeft:
            return "bottom_left";
        case UiOverlayAnchor::BottomRight:
            return "bottom_right";
        case UiOverlayAnchor::TopRight:
        default:
            return "top_right";
    }
}

HitSoundVariant
SettingsManager::_ParseHitSoundVariant(const std::string& value) noexcept {
    if (value == "click")   return HitSoundVariant::Click;
    if (value == "gunshot") return HitSoundVariant::Gunshot;
    if (value == "pop")     return HitSoundVariant::Pop;
    return HitSoundVariant::Swish;
}

MissSoundVariant
SettingsManager::_ParseMissSoundVariant(const std::string& value) noexcept {
    if (value == "empty")    return MissSoundVariant::Empty;
    if (value == "beep")     return MissSoundVariant::Beep;
    if (value == "tap")      return MissSoundVariant::Tap;
    if (value == "miss_pop") return MissSoundVariant::MissPop;
    if (value == "oops")     return MissSoundVariant::Oops;
    return MissSoundVariant::Ricochet;
}

std::string SettingsManager::_ToStorageValue(HitSoundVariant variant) {
    switch (variant) {
        case HitSoundVariant::Click:   return "click";
        case HitSoundVariant::Gunshot: return "gunshot";
        case HitSoundVariant::Pop:     return "pop";
        case HitSoundVariant::Swish:
        default:                       return "swish";
    }
}

std::string SettingsManager::_ToStorageValue(MissSoundVariant variant) {
    switch (variant) {
        case MissSoundVariant::Empty:    return "empty";
        case MissSoundVariant::Beep:     return "beep";
        case MissSoundVariant::Tap:      return "tap";
        case MissSoundVariant::MissPop:  return "miss_pop";
        case MissSoundVariant::Oops:     return "oops";
        case MissSoundVariant::Ricochet:
        default:                         return "ricochet";
    }
}
}  // namespace xaimassist::persistence
