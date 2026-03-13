/**
 * @file AppSettings.hpp
 * @brief Aggregate settings POD hierarchy for the entire application.
 */

#ifndef APPSETTINGS_HPP
#define APPSETTINGS_HPP

#include <string>
#include <unordered_map>

namespace xaimassist::persistence {

enum class UiThemeMode { Dark,
                         Light };
enum class UiLanguage { English,
                        Turkish };
enum class UiOverlayAnchor { TopLeft,
                             TopRight,
                             BottomLeft,
                             BottomRight };

/// Normalised RGB colour with components in [0, 1].
struct ColorRgb {
    double red{0.95};
    double green{0.35};
    double blue{0.25};
};

/// Physical-first sensitivity parameters.
struct SensitivityPreferences {
    double cmPer360{34.0};
    double dpi{800.0};
    double sensitivityScale{1.0};
    double yawMultiplier{1.0};
    double pitchMultiplier{1.0};
    double scopedMultiplier{0.8};
    bool invertY{false};
};

struct InputPreferences {
    bool rawInputEnabled{true};
    SensitivityPreferences sensitivity;
};

struct TargetPreferences {
    ColorRgb color;
    double radius{0.5};
};

/// Per-mode overrides stored in the profile.
struct ModePreferences {
    double durationSeconds{0.0};
    double distanceUnits{0.0};
    std::unordered_map<std::string, double> settingValues;
};

struct GameplayPreferences {
    TargetPreferences target;
    std::string selectedModeId{"static_sphere"};
    std::unordered_map<std::string, ModePreferences> modeOverrides;
};

/// Crosshair visual parameters.
struct CrosshairPreferences {
    double thickness{2.0};
    bool centerDotEnabled{true};
    double centerDotSize{4.0};
    ColorRgb color{0.95, 0.97, 0.99};
    bool linesEnabled{true};
    bool borderEnabled{true};
    double borderThickness{1.0};
    double horizontalLength{10.0};
    double verticalLength{10.0};
    double gap{4.0};
};

struct FpsCounterPreferences {
    bool enabled{false};
    UiOverlayAnchor anchor{UiOverlayAnchor::TopRight};
};

struct UiPreferences {
    UiThemeMode themeMode{UiThemeMode::Dark};
    UiLanguage language{UiLanguage::English};
    CrosshairPreferences crosshair;
    FpsCounterPreferences fpsCounter;
};

/// Root aggregate of all persisted application settings.
struct AppSettings {
    InputPreferences input;
    GameplayPreferences gameplay;
    UiPreferences ui;
};
}  // namespace xaimassist::persistence

#endif  // APPSETTINGS_HPP
