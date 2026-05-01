/**
 * @file UiViewModel.hpp
 * @brief Central QML ViewModel binding UI screens to application state.
 *
 * Exposes Q_PROPERTYs for all settings, session stats, mode selection,
 * training overlay state, and theme/i18n data consumed by QML.
 */

#ifndef UIVIEWMODEL_HPP
#define UIVIEWMODEL_HPP

#include <QObject>
#include <QString>
#include <QVariant>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/CoreEvents.hpp"
#include "persistence/AppSettings.hpp"
#include "persistence/SessionHistory.hpp"

namespace xaimassist::ui {
/**
 * @class UiViewModel
 * @brief QObject ViewModel bridging QML UI and C++ application services.
 *
 * All user-visible state (modes, stats, settings, overlays) is
 * surfaced as Q_PROPERTYs, and user commands are forwarded via
 * typed callbacks to the application layer.
 */
class UiViewModel final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantMap theme READ GetTheme NOTIFY themeChanged)
    Q_PROPERTY(QVariantMap i18n READ GetI18n NOTIFY translationsChanged)
    Q_PROPERTY(QVariantList modes READ GetModes NOTIFY modesChanged)
    Q_PROPERTY(
        QVariantList modeSettings READ GetModeSettings NOTIFY modeSettingsChanged)
    Q_PROPERTY(QVariantList recentSessions READ GetRecentSessions NOTIFY
                   recentSessionsChanged)
    Q_PROPERTY(
        QVariantMap bestSession READ GetBestSession NOTIFY bestSessionChanged)
    Q_PROPERTY(QVariantMap realtimeStats READ GetRealtimeStats NOTIFY
                   realtimeStatsChanged)
    Q_PROPERTY(QVariantMap performanceStats READ GetPerformanceStats NOTIFY
                   performanceStatsChanged)
    Q_PROPERTY(
        QVariantMap latestResult READ GetLatestResult NOTIFY latestResultChanged)

    Q_PROPERTY(QString currentScreen READ GetCurrentScreen WRITE SetCurrentScreen
                   NOTIFY currentScreenChanged)
    Q_PROPERTY(QString selectedModeId READ GetSelectedModeId WRITE
                   SetSelectedModeId NOTIFY selectedModeChanged)
    Q_PROPERTY(double modeDurationSeconds READ GetModeDurationSeconds WRITE
                   SetModeDurationSeconds NOTIFY modeDurationChanged)
    Q_PROPERTY(double modeTargetDistance READ GetModeTargetDistance WRITE
                   SetModeTargetDistance NOTIFY modeTargetDistanceChanged)
    Q_PROPERTY(int modeGridRows READ GetModeGridRows WRITE SetModeGridRows NOTIFY
                   modeGridRowsChanged)
    Q_PROPERTY(int modeGridColumns READ GetModeGridColumns WRITE
                   SetModeGridColumns NOTIFY modeGridColumnsChanged)
    Q_PROPERTY(int modeActiveTargetCount READ GetModeActiveTargetCount WRITE
                   SetModeActiveTargetCount NOTIFY modeActiveTargetCountChanged)
    Q_PROPERTY(bool modeGridConfigAvailable READ GetModeGridConfigAvailable NOTIFY
                   modeGridConfigChanged)
    Q_PROPERTY(int modeGridCellCount READ GetModeGridCellCount NOTIFY
                   modeGridConfigChanged)
    Q_PROPERTY(int modeActiveTargetMax READ GetModeActiveTargetMax NOTIFY
                   modeGridConfigChanged)
    Q_PROPERTY(
        bool sessionActive READ GetSessionActive NOTIFY sessionStateChanged)
    Q_PROPERTY(
        QString activeModeId READ GetActiveModeId NOTIFY sessionStateChanged)
    Q_PROPERTY(
        bool sceneVisible READ GetSceneVisible NOTIFY trainingOverlayChanged)
    Q_PROPERTY(bool waitingForSceneClick READ GetWaitingForSceneClick NOTIFY
                   trainingOverlayChanged)
    Q_PROPERTY(bool countdownVisible READ GetCountdownVisible NOTIFY
                   trainingOverlayChanged)
    Q_PROPERTY(
        int countdownValue READ GetCountdownValue NOTIFY trainingOverlayChanged)
    Q_PROPERTY(bool pauseMenuVisible READ GetPauseMenuVisible NOTIFY
                   trainingOverlayChanged)
    Q_PROPERTY(bool crosshairVisible READ GetCrosshairVisible NOTIFY
                   trainingOverlayChanged)
    Q_PROPERTY(double currentFps READ GetCurrentFps NOTIFY fpsChanged)
    Q_PROPERTY(bool fpsCounterEnabled READ GetFpsCounterEnabled WRITE
                   SetFpsCounterEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString fpsCounterPosition READ GetFpsCounterPosition WRITE
                   SetFpsCounterPosition NOTIFY settingsChanged)

    Q_PROPERTY(double crosshairThickness READ GetCrosshairThickness WRITE
                   SetCrosshairThickness NOTIFY settingsChanged)
    Q_PROPERTY(bool crosshairCenterDotEnabled READ GetCrosshairCenterDotEnabled
                   WRITE SetCrosshairCenterDotEnabled NOTIFY settingsChanged)
    Q_PROPERTY(double crosshairCenterDotSize READ GetCrosshairCenterDotSize WRITE
                   SetCrosshairCenterDotSize NOTIFY settingsChanged)
    Q_PROPERTY(double crosshairColorRed READ GetCrosshairColorRed WRITE
                   SetCrosshairColorRed NOTIFY settingsChanged)
    Q_PROPERTY(double crosshairColorGreen READ GetCrosshairColorGreen WRITE
                   SetCrosshairColorGreen NOTIFY settingsChanged)
    Q_PROPERTY(double crosshairColorBlue READ GetCrosshairColorBlue WRITE
                   SetCrosshairColorBlue NOTIFY settingsChanged)
    Q_PROPERTY(bool crosshairLinesEnabled READ GetCrosshairLinesEnabled WRITE
                   SetCrosshairLinesEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool crosshairBorderEnabled READ GetCrosshairBorderEnabled WRITE
                   SetCrosshairBorderEnabled NOTIFY settingsChanged)
    Q_PROPERTY(double crosshairBorderThickness READ GetCrosshairBorderThickness
                   WRITE SetCrosshairBorderThickness NOTIFY settingsChanged)
    Q_PROPERTY(double crosshairHorizontalLength READ GetCrosshairHorizontalLength
                   WRITE SetCrosshairHorizontalLength NOTIFY settingsChanged)
    Q_PROPERTY(double crosshairVerticalLength READ GetCrosshairVerticalLength
                   WRITE SetCrosshairVerticalLength NOTIFY settingsChanged)
    Q_PROPERTY(double crosshairGap READ GetCrosshairGap WRITE SetCrosshairGap
                   NOTIFY settingsChanged)

    Q_PROPERTY(bool rawInputEnabled READ GetRawInputEnabled WRITE
                   SetRawInputEnabled NOTIFY settingsChanged)
    Q_PROPERTY(
        double cmPer360 READ GetCmPer360 WRITE SetCmPer360 NOTIFY settingsChanged)
    Q_PROPERTY(double dpi READ GetDpi WRITE SetDpi NOTIFY settingsChanged)
    Q_PROPERTY(double sensitivityScale READ GetSensitivityScale WRITE
                   SetSensitivityScale NOTIFY settingsChanged)
    Q_PROPERTY(double yawMultiplier READ GetYawMultiplier WRITE SetYawMultiplier
                   NOTIFY settingsChanged)
    Q_PROPERTY(double pitchMultiplier READ GetPitchMultiplier WRITE
                   SetPitchMultiplier NOTIFY settingsChanged)
    Q_PROPERTY(
        bool invertY READ GetInvertY WRITE SetInvertY NOTIFY settingsChanged)

    Q_PROPERTY(double targetColorRed READ GetTargetColorRed WRITE
                   SetTargetColorRed NOTIFY settingsChanged)
    Q_PROPERTY(double targetColorGreen READ GetTargetColorGreen WRITE
                   SetTargetColorGreen NOTIFY settingsChanged)
    Q_PROPERTY(double targetColorBlue READ GetTargetColorBlue WRITE
                   SetTargetColorBlue NOTIFY settingsChanged)
    Q_PROPERTY(double targetRadius READ GetTargetRadius WRITE SetTargetRadius
                   NOTIFY settingsChanged)

    Q_PROPERTY(QString themeMode READ GetThemeMode WRITE SetThemeMode NOTIFY
                   settingsChanged)
    Q_PROPERTY(QString languageCode READ GetLanguageCode WRITE SetLanguageCode
                   NOTIFY settingsChanged)

    Q_PROPERTY(bool startFullscreen READ GetStartFullscreen WRITE
                   SetStartFullscreen NOTIFY settingsChanged)
    Q_PROPERTY(QString keybindToggleFullscreen READ GetKeybindToggleFullscreen
                   WRITE SetKeybindToggleFullscreen NOTIFY settingsChanged)
    Q_PROPERTY(QString keybindToggleFpsCounter READ GetKeybindToggleFpsCounter
                   WRITE SetKeybindToggleFpsCounter NOTIFY settingsChanged)
    Q_PROPERTY(QString keybindToggleCrosshair READ GetKeybindToggleCrosshair
                   WRITE SetKeybindToggleCrosshair NOTIFY settingsChanged)

    Q_PROPERTY(bool soundEnabled READ GetSoundEnabled WRITE SetSoundEnabled
                   NOTIFY settingsChanged)
    Q_PROPERTY(double soundVolume READ GetSoundVolume WRITE SetSoundVolume
                   NOTIFY settingsChanged)
    Q_PROPERTY(QString soundHitVariant READ GetSoundHitVariant WRITE
                   SetSoundHitVariant NOTIFY settingsChanged)
    Q_PROPERTY(QString soundMissVariant READ GetSoundMissVariant WRITE
                   SetSoundMissVariant NOTIFY settingsChanged)

    Q_PROPERTY(bool crosshairKeyToggleEnabled READ GetCrosshairKeyToggleEnabled
                   NOTIFY trainingOverlayChanged)

  public:
    /// Descriptor mirroring GameModeSettingMetadata for UI consumption.
    struct ModeSettingDescriptor {
        std::string key;
        std::string displayName;
        double defaultValue{0.0};
        double minValue{0.0};
        double maxValue{0.0};
        double step{1.0};
        bool integerOnly{false};
    };

    /// Descriptor mirroring GameModeMetadata for UI consumption.
    struct ModeDescriptor {
        std::string id;
        std::string displayName;
        std::string description;
        double defaultDurationSeconds{60.0};
        double defaultDistanceUnits{25.0};
        int defaultGridRows{0};
        int defaultGridColumns{0};
        int defaultActiveTargetCount{0};
        std::vector<ModeSettingDescriptor> settings;
    };

    /// Result-screen benchmark comparison for a single metric.
    struct PerformanceMetricComparison {
        std::string metricId;
        std::string tier;
        double value{0.0};
        double average{0.0};
        double best{0.0};
        double worst{0.0};
    };

    using SettingsChangedCallback = std::function<void(const persistence::AppSettings& settings, const std::array<double, 3>& viewportBackground)>;
    using StartModeCallback = std::function<void(const std::string& modeId, double durationSeconds, double distanceUnits, const std::unordered_map<std::string, double>& modeSettings)>;
    using StopModeCallback = std::function<void()>;
    using ContinueTrainingCallback = std::function<void()>;
    using ExitTrainingCallback = std::function<void()>;
    using ModeSelectionChangedCallback = std::function<void(const std::string& modeId)>;

    explicit UiViewModel(persistence::AppSettings settings, QObject* parent = nullptr);

    /// Register callback invoked when settings are changed from the UI.
    void SetSettingsChangedCallback(SettingsChangedCallback callback);

    /// Register callback invoked when the user starts a mode.
    void SetStartModeCallback(StartModeCallback callback);

    /// Register callback invoked when the user stops the active mode.
    void SetStopModeCallback(StopModeCallback callback);

    /// Register callback invoked when the user continues training after pause.
    void SetContinueTrainingCallback(ContinueTrainingCallback callback);

    /// Register callback invoked when the user exits training.
    void SetExitTrainingCallback(ExitTrainingCallback callback);

    /// Register callback invoked when the mode selection changes.
    void SetModeSelectionChangedCallback(ModeSelectionChangedCallback callback);

    /// Populate the available game modes list.
    void SetModes(const std::vector<ModeDescriptor>& modes);

    /// Update session history data for the statistics screen.
    void SetSessionHistory(
        const std::vector<persistence::SessionRecord>& recentSessions,
        const std::optional<persistence::SessionRecord>& bestSessionForSelectedMode);

    /// Forward a frame tick for FPS calculation.
    void UpdateFrameTick(const core::events::FrameTickEvent& event);

    /// Push real-time stats to the QML overlay.
    void UpdateRealtimeStats(const core::events::RealtimeStatsEvent& event);

    /// Push performance metrics to the QML overlay.
    void UpdatePerformanceStats(const core::events::PerformanceStatsEvent& event);

    /// Notify the UI that a new session has started.
    void OnSessionStarted(const core::events::SessionStartedEvent& event);

    /// Notify the UI that the active session has stopped.
    void OnSessionStopped(const core::events::SessionStoppedEvent& event);

    /// Set the benchmark comparison data shown on the result screen.
    void SetLatestResultPerformanceBenchmark(
        std::uint64_t sessionId, const std::string& overallCategory,
        std::uint64_t modeSessionCount,
        const std::vector<PerformanceMetricComparison>& metricComparisons);

    /// Mark the latest result as invalid (e.g. too short to score).
    void SetLatestResultInvalid(std::uint64_t sessionId, const std::string& reasonCode);

    /// In-memory settings snapshot.
    const persistence::AppSettings& GetSettings() const noexcept;

    /// Viewport background colour derived from the active theme.
    std::array<double, 3> GetViewportBackgroundColor() const noexcept;

    /// Current selected mode id as a std::string.
    std::string GetSelectedModeIdStd() const;

    /// Theme colour palette for QML.
    QVariantMap GetTheme() const;

    /// Localised string table for the current language.
    QVariantMap GetI18n() const;

    /// List of available game mode descriptors.
    QVariantList GetModes() const;

    /// Settings descriptors for the currently selected mode.
    QVariantList GetModeSettings() const;

    /// Recent training sessions for the statistics screen.
    QVariantList GetRecentSessions() const;

    /// Best session record for the selected mode.
    QVariantMap GetBestSession() const;

    /// Live stats during a training session.
    QVariantMap GetRealtimeStats() const;

    /// Performance stats (FPS, frame times).
    QVariantMap GetPerformanceStats() const;

    /// Latest session result with benchmark comparisons.
    QVariantMap GetLatestResult() const;

    /// Active QML screen name.
    QString GetCurrentScreen() const;
    void SetCurrentScreen(const QString& screen);

    /// Currently selected mode id (QString).
    QString GetSelectedModeId() const;
    void SetSelectedModeId(const QString& modeId);

    /// Session duration override in seconds.
    double GetModeDurationSeconds() const;
    void SetModeDurationSeconds(double value);

    /// Target distance override in world units.
    double GetModeTargetDistance() const;
    void SetModeTargetDistance(double value);

    /// Grid row count for grid-based modes.
    int GetModeGridRows() const;
    void SetModeGridRows(int value);

    /// Grid column count for grid-based modes.
    int GetModeGridColumns() const;
    void SetModeGridColumns(int value);

    /// Number of simultaneously active targets on the grid.
    int GetModeActiveTargetCount() const;
    void SetModeActiveTargetCount(int value);

    /// True if the selected mode supports grid configuration.
    bool GetModeGridConfigAvailable() const;

    /// Total cell count for the current grid layout.
    int GetModeGridCellCount() const;

    /// Maximum allowed active targets for the current grid.
    int GetModeActiveTargetMax() const;

    /// True if a training session is currently active.
    bool GetSessionActive() const;

    /// Mode id of the running session.
    QString GetActiveModeId() const;

    /// True if the 3D scene should be visible.
    bool GetSceneVisible() const;

    /// True if the UI is waiting for the first scene click.
    bool GetWaitingForSceneClick() const;

    /// True if the countdown overlay is displayed.
    bool GetCountdownVisible() const;

    /// Current countdown integer value.
    int GetCountdownValue() const;

    /// True if the pause menu overlay is visible.
    bool GetPauseMenuVisible() const;

    /// True if the crosshair overlay is drawn.
    bool GetCrosshairVisible() const;

    /// Smoothed frames-per-second value.
    double GetCurrentFps() const;

    /// Whether the on-screen FPS counter is enabled.
    bool GetFpsCounterEnabled() const;
    void SetFpsCounterEnabled(bool enabled);

    /// Anchor position of the FPS counter (e.g. "TopRight").
    QString GetFpsCounterPosition() const;
    void SetFpsCounterPosition(const QString& position);

    /// Crosshair line thickness in pixels.
    double GetCrosshairThickness() const;
    void SetCrosshairThickness(double value);

    /// Whether the crosshair centre dot is drawn.
    bool GetCrosshairCenterDotEnabled() const;
    void SetCrosshairCenterDotEnabled(bool enabled);

    /// Centre dot size in pixels.
    double GetCrosshairCenterDotSize() const;
    void SetCrosshairCenterDotSize(double value);

    /// Crosshair colour red channel [0, 1].
    double GetCrosshairColorRed() const;
    void SetCrosshairColorRed(double value);

    /// Crosshair colour green channel [0, 1].
    double GetCrosshairColorGreen() const;
    void SetCrosshairColorGreen(double value);

    /// Crosshair colour blue channel [0, 1].
    double GetCrosshairColorBlue() const;
    void SetCrosshairColorBlue(double value);

    /// Whether the crosshair arm lines are drawn.
    bool GetCrosshairLinesEnabled() const;
    void SetCrosshairLinesEnabled(bool enabled);

    /// Whether the crosshair border outline is drawn.
    bool GetCrosshairBorderEnabled() const;
    void SetCrosshairBorderEnabled(bool enabled);

    /// Crosshair border thickness in pixels.
    double GetCrosshairBorderThickness() const;
    void SetCrosshairBorderThickness(double value);

    /// Horizontal crosshair arm length in pixels.
    double GetCrosshairHorizontalLength() const;
    void SetCrosshairHorizontalLength(double value);

    /// Vertical crosshair arm length in pixels.
    double GetCrosshairVerticalLength() const;
    void SetCrosshairVerticalLength(double value);

    /// Gap between crosshair arms and centre dot.
    double GetCrosshairGap() const;
    void SetCrosshairGap(double value);

    /// Whether raw (unaccelerated) mouse input is used.
    bool GetRawInputEnabled() const;
    void SetRawInputEnabled(bool enabled);

    /// Physical distance (cm) for a full 360-degree turn.
    double GetCmPer360() const;
    void SetCmPer360(double value);

    /// Mouse hardware DPI.
    double GetDpi() const;
    void SetDpi(double value);

    /// Additional sensitivity multiplier.
    double GetSensitivityScale() const;
    void SetSensitivityScale(double value);

    /// Yaw axis sensitivity multiplier.
    double GetYawMultiplier() const;
    void SetYawMultiplier(double value);

    /// Pitch axis sensitivity multiplier.
    double GetPitchMultiplier() const;
    void SetPitchMultiplier(double value);

    /// Whether vertical mouse axis is inverted.
    bool GetInvertY() const;
    void SetInvertY(bool enabled);

    /// Target colour red channel [0, 1].
    double GetTargetColorRed() const;
    void SetTargetColorRed(double value);

    /// Target colour green channel [0, 1].
    double GetTargetColorGreen() const;
    void SetTargetColorGreen(double value);

    /// Target colour blue channel [0, 1].
    double GetTargetColorBlue() const;
    void SetTargetColorBlue(double value);

    /// Target sphere radius in world units.
    double GetTargetRadius() const;
    void SetTargetRadius(double value);

    /// Current UI theme mode ("dark" or "light").
    QString GetThemeMode() const;
    void SetThemeMode(const QString& mode);

    /// Current UI language code ("en" or "tr").
    QString GetLanguageCode() const;
    void SetLanguageCode(const QString& code);

    bool GetStartFullscreen() const;
    void SetStartFullscreen(bool value);

    bool GetSoundEnabled() const;
    void SetSoundEnabled(bool enabled);

    double GetSoundVolume() const;
    void SetSoundVolume(double value);

    QString GetSoundHitVariant() const;
    void SetSoundHitVariant(const QString& variant);

    QString GetSoundMissVariant() const;
    void SetSoundMissVariant(const QString& variant);

    QString GetKeybindToggleFullscreen() const;
    void SetKeybindToggleFullscreen(const QString& key);

    QString GetKeybindToggleFpsCounter() const;
    void SetKeybindToggleFpsCounter(const QString& key);

    QString GetKeybindToggleCrosshair() const;
    void SetKeybindToggleCrosshair(const QString& key);

    bool GetCrosshairKeyToggleEnabled() const;
    void ToggleCrosshairKeyOverride();
    void ToggleFpsCounterRuntime();

    /// Start the currently selected mode with configured overrides.
    Q_INVOKABLE void RequestStartSelectedMode();

    /// Update a mode-specific setting value.
    Q_INVOKABLE void SetModeSettingValue(const QString& settingKey, double value);

    /// Reset all settings for the selected mode to their defaults.
    Q_INVOKABLE void ResetSelectedModeSettingsToDefaults();

    /// Request stopping the active training session.
    Q_INVOKABLE void RequestStopMode();

    /// Resume training from the pause menu.
    Q_INVOKABLE void RequestContinueTraining();

    /// Exit training and return to the main menu.
    Q_INVOKABLE void RequestExitTraining();

    /// Navigate to the settings screen.
    Q_INVOKABLE void RequestOpenSettings();

    /// Return from settings to the previous screen.
    Q_INVOKABLE void RequestBackFromSettings();

    /// Transition to the scene-visible state awaiting a trigger click.
    void ShowTrainingSceneAwaitingTrigger();

    /// Show the countdown overlay with the given seconds remaining.
    void ShowTrainingCountdown(int secondsRemaining);

    /// Transition to the fully-active training overlay.
    void ShowTrainingActive();

    /// Show the pause menu overlay.
    void ShowTrainingPaused();

    /// Return to the main menu screen.
    void ReturnToMainMenu();

  signals:
    void themeChanged();
    void translationsChanged();
    void modesChanged();
    void modeSettingsChanged();
    void recentSessionsChanged();
    void bestSessionChanged();
    void realtimeStatsChanged();
    void performanceStatsChanged();
    void latestResultChanged();
    void currentScreenChanged();
    void selectedModeChanged();
    void modeDurationChanged();
    void modeTargetDistanceChanged();
    void modeGridRowsChanged();
    void modeGridColumnsChanged();
    void modeActiveTargetCountChanged();
    void modeGridConfigChanged();
    void sessionStateChanged();
    void trainingOverlayChanged();
    void settingsChanged();
    void fpsChanged();

  private:
    static double _Clamp01(double value) noexcept;
    static bool _NearlyEqual(double left, double right) noexcept;

    static persistence::UiThemeMode _ParseThemeMode(const QString& mode);
    static QString _ToThemeModeString(persistence::UiThemeMode mode);
    static persistence::UiLanguage _ParseLanguageCode(const QString& code);
    static QString _ToLanguageCodeString(persistence::UiLanguage language);
    static persistence::UiOverlayAnchor _ParseOverlayAnchorCode(const QString& code);
    static QString _ToOverlayAnchorCode(persistence::UiOverlayAnchor anchor);

    static std::array<double, 3> _Mix(const std::array<double, 3>& left, const std::array<double, 3>& right, double factor);
    static double _Luminance(const std::array<double, 3>& color) noexcept;
    static QString _ToHexColor(const std::array<double, 3>& color);

    QString _LocalizedModeName(const ModeDescriptor& mode) const;
    QString _LocalizedModeDescription(const ModeDescriptor& mode) const;
    QString _LocalizedModeSettingName(const std::string& modeId, const ModeSettingDescriptor& setting) const;
    double _SelectedModeDefaultDurationSeconds() const;
    double _SelectedModeDefaultDistanceUnits() const;
    int _SelectedModeDefaultGridRows() const;
    int _SelectedModeDefaultGridColumns() const;
    int _SelectedModeDefaultActiveTargetCount() const;
    bool _SelectedModeSupportsGridConfig() const;
    int _ClampedActiveTargetCountForGrid(int value, int rows, int columns) const;
    const ModeDescriptor* _SelectedModeDescriptor() const;
    void _ResetModeSettingsForSelection();
    void _ApplySelectedModePreferences();
    void _PersistSelectedModePreferences();
    void _RebuildModeSettingsVariant();

    void _RebuildTheme();
    void _RebuildI18n();
    void _RebuildModesVariant();
    void _EmitSettingsApplied();
    void _SetKeybindField(std::string& target, const QString& key);

    QVariantMap _ToSessionVariant(const persistence::SessionRecord& record) const;
    static QString _FormatIsoDateTime(const std::string& iso);

    persistence::AppSettings m_settings;

    QVariantMap m_theme;
    QVariantMap m_i18n;
    std::array<double, 3> m_viewportBackgroundColor{0.05, 0.08, 0.16};

    std::vector<ModeDescriptor> m_modes;
    QVariantList m_modeList;
    QVariantList m_modeSettingsList;
    std::unordered_map<std::string, double> m_modeSettingValues;

    QVariantList m_recentSessions;
    QVariantMap m_bestSession;
    QVariantMap m_realtimeStats;
    QVariantMap m_performanceStats;
    QVariantMap m_latestResult;

    QString m_currentScreen{"menu"};
    QString m_selectedModeId{"static_sphere"};
    double m_modeDurationSeconds{60.0};
    double m_modeTargetDistance{25.0};
    int m_modeGridRows{0};
    int m_modeGridColumns{0};
    int m_modeActiveTargetCount{0};
    bool m_sessionActive{false};
    QString m_activeModeId;
    bool m_sceneVisible{false};
    bool m_waitingForSceneClick{false};
    int m_countdownValue{0};
    bool m_pauseMenuVisible{false};
    bool m_crosshairVisible{false};
    bool m_crosshairKeyToggle{true};
    double m_currentFps{0.0};
    double m_fpsAccumulatorSeconds{0.0};
    int m_fpsAccumulatorFrames{0};

    SettingsChangedCallback m_settingsChangedCallback;
    StartModeCallback m_startModeCallback;
    StopModeCallback m_stopModeCallback;
    ContinueTrainingCallback m_continueTrainingCallback;
    ExitTrainingCallback m_exitTrainingCallback;
    ModeSelectionChangedCallback m_modeSelectionChangedCallback;
};
}  // namespace xaimassist::ui

#endif  // UIVIEWMODEL_HPP
