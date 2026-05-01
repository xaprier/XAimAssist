/// @file UiViewModel.cpp
#include "ui/UiViewModel.hpp"

#include <QColor>
#include <QDateTime>
#include <QTimeZone>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <utility>

#include "ui/I18nProvider.hpp"
#include "ui/Version.hpp"

namespace {
constexpr std::array<const char*, 6> SCREEN_IDS = {
    "menu",
    "modes",
    "training",
    "stats",
    "settings",
    "about",
};

bool isSupportedScreen(const QString& screen) {
    for (const auto* screenId : SCREEN_IDS) {
        if (screen == QString::fromUtf8(screenId)) {
            return true;
        }
    }

    return false;
}
}  // namespace

namespace xaimassist::ui {
UiViewModel::UiViewModel(persistence::AppSettings settings, QObject* parent)
    : QObject(parent), m_settings(std::move(settings)) {
    if (!m_settings.gameplay.selectedModeId.empty()) {
        m_selectedModeId =
            QString::fromStdString(m_settings.gameplay.selectedModeId);
    } else {
        m_settings.gameplay.selectedModeId = m_selectedModeId.toStdString();
    }

    _RebuildI18n();
    _RebuildTheme();
    _RebuildModesVariant();
    _RebuildModeSettingsVariant();

    m_realtimeStats = {
        {"modeId", QString::fromUtf8("-")},
        {"elapsedSeconds", 0.0},
        {"score", static_cast<qlonglong>(0)},
        {"hits", static_cast<qlonglong>(0)},
        {"misses", static_cast<qlonglong>(0)},
        {"shotsFired", static_cast<qlonglong>(0)},
        {"accuracyPercent", 0.0},
        {"averageReactionTimeMs", 0.0},
        {"shotsPerSecond", 0.0},
    };

    m_performanceStats = {
        {"windowSeconds", 0.0},
        {"sampledFrames", static_cast<qlonglong>(0)},
        {"fps", 0.0},
        {"averageFrameTimeMs", 0.0},
        {"p95FrameTimeMs", 0.0},
        {"worstFrameTimeMs", 0.0},
        {"frameJitterMs", 0.0},
        {"simulationLoadPercent", 0.0},
        {"missedTargetFramePercent", 0.0},
    };
}

void UiViewModel::SetSettingsChangedCallback(SettingsChangedCallback callback) {
    m_settingsChangedCallback = std::move(callback);
}

void UiViewModel::SetStartModeCallback(StartModeCallback callback) {
    m_startModeCallback = std::move(callback);
}

void UiViewModel::SetStopModeCallback(StopModeCallback callback) {
    m_stopModeCallback = std::move(callback);
}

void UiViewModel::SetContinueTrainingCallback(
    ContinueTrainingCallback callback) {
    m_continueTrainingCallback = std::move(callback);
}

void UiViewModel::SetExitTrainingCallback(ExitTrainingCallback callback) {
    m_exitTrainingCallback = std::move(callback);
}

void UiViewModel::SetModeSelectionChangedCallback(
    ModeSelectionChangedCallback callback) {
    m_modeSelectionChangedCallback = std::move(callback);
}

void UiViewModel::SetModes(const std::vector<ModeDescriptor>& modes) {
    m_modes = modes;
    bool selectedModeChangedFallback = false;
    const double previousModeDurationSeconds = m_modeDurationSeconds;
    const double previousModeDistanceUnits = m_modeTargetDistance;
    const int previousModeGridRows = m_modeGridRows;
    const int previousModeGridColumns = m_modeGridColumns;
    const int previousModeActiveTargetCount = m_modeActiveTargetCount;
    const bool previousGridConfigAvailable = GetModeGridConfigAvailable();
    const int previousGridCellCount = GetModeGridCellCount();
    const int previousActiveTargetMax = GetModeActiveTargetMax();

    if (!m_modes.empty()) {
        const bool selectedModeIsKnown = std::any_of(
            m_modes.begin(), m_modes.end(), [&](const ModeDescriptor& mode) {
                return m_selectedModeId == QString::fromStdString(mode.id);
            });

        if (!selectedModeIsKnown) {
            m_selectedModeId = QString::fromStdString(m_modes.front().id);
            selectedModeChangedFallback = true;
            emit selectedModeChanged();
        }
    }

    const std::string selectedModeIdString = GetSelectedModeIdStd();
    if (m_settings.gameplay.selectedModeId != selectedModeIdString) {
        m_settings.gameplay.selectedModeId = selectedModeIdString;
        if (selectedModeChangedFallback) {
            _EmitSettingsApplied();
        }
    }

    m_modeDurationSeconds =
        std::clamp(_SelectedModeDefaultDurationSeconds(), 10.0, 600.0);
    m_modeTargetDistance =
        std::clamp(_SelectedModeDefaultDistanceUnits(), 1.0, 150.0);

    if (_SelectedModeSupportsGridConfig()) {
        m_modeGridRows = std::clamp(_SelectedModeDefaultGridRows(), 2, 15);
        m_modeGridColumns = std::clamp(_SelectedModeDefaultGridColumns(), 2, 15);
        m_modeActiveTargetCount = _ClampedActiveTargetCountForGrid(
            _SelectedModeDefaultActiveTargetCount(), m_modeGridRows,
            m_modeGridColumns);
    } else {
        m_modeGridRows = 0;
        m_modeGridColumns = 0;
        m_modeActiveTargetCount = 0;
    }

    _ResetModeSettingsForSelection();
    _ApplySelectedModePreferences();

    _RebuildModesVariant();
    _RebuildModeSettingsVariant();
    emit modesChanged();
    emit modeSettingsChanged();

    if (!_NearlyEqual(previousModeDurationSeconds, m_modeDurationSeconds)) {
        emit modeDurationChanged();
    }

    if (!_NearlyEqual(previousModeDistanceUnits, m_modeTargetDistance)) {
        emit modeTargetDistanceChanged();
    }

    if (previousModeGridRows != m_modeGridRows) {
        emit modeGridRowsChanged();
    }

    if (previousModeGridColumns != m_modeGridColumns) {
        emit modeGridColumnsChanged();
    }

    if (previousModeActiveTargetCount != m_modeActiveTargetCount) {
        emit modeActiveTargetCountChanged();
    }

    if (previousGridConfigAvailable != GetModeGridConfigAvailable() ||
        previousGridCellCount != GetModeGridCellCount() ||
        previousActiveTargetMax != GetModeActiveTargetMax()) {
        emit modeGridConfigChanged();
    }

    if (m_modeSelectionChangedCallback) {
        m_modeSelectionChangedCallback(GetSelectedModeIdStd());
    }
}

void UiViewModel::SetSessionHistory(
    const std::vector<persistence::SessionRecord>& recentSessions,
    const std::optional<persistence::SessionRecord>& bestSessionForSelectedMode) {
    QVariantList convertedRecentSessions;
    for (const auto& record : recentSessions) {
        convertedRecentSessions.push_back(_ToSessionVariant(record));
    }

    m_recentSessions = convertedRecentSessions;
    emit recentSessionsChanged();

    if (bestSessionForSelectedMode.has_value()) {
        m_bestSession = _ToSessionVariant(*bestSessionForSelectedMode);
    } else {
        m_bestSession.clear();
    }

    emit bestSessionChanged();
}

void UiViewModel::UpdateFrameTick(const core::events::FrameTickEvent& event) {
    if (event.frameSeconds <= 0.0 || !m_settings.ui.fpsCounter.enabled) {
        return;
    }

    m_fpsAccumulatorSeconds += event.frameSeconds;
    m_fpsAccumulatorFrames += 1;

    if (m_fpsAccumulatorSeconds < 0.2) {
        return;
    }

    const double fps =
        static_cast<double>(m_fpsAccumulatorFrames) / m_fpsAccumulatorSeconds;
    m_fpsAccumulatorSeconds = 0.0;
    m_fpsAccumulatorFrames = 0;

    if (_NearlyEqual(fps, m_currentFps)) {
        return;
    }

    m_currentFps = fps;
    emit fpsChanged();
}

void UiViewModel::UpdateRealtimeStats(
    const core::events::RealtimeStatsEvent& event) {
    bool changed = false;
    const auto setField = [&](const QString& key, const QVariant& value) {
        const QVariant currentValue = m_realtimeStats.value(key);
        if (currentValue == value) {
            return;
        }

        m_realtimeStats.insert(key, value);
        changed = true;
    };

    setField(QStringLiteral("modeId"), QString::fromStdString(event.modeId));
    setField(QStringLiteral("elapsedSeconds"), event.elapsedSeconds);
    setField(QStringLiteral("score"), static_cast<qlonglong>(event.score));
    setField(QStringLiteral("hits"), static_cast<qlonglong>(event.hits));
    setField(QStringLiteral("misses"), static_cast<qlonglong>(event.misses));
    setField(QStringLiteral("shotsFired"),
             static_cast<qlonglong>(event.shotsFired));
    setField(QStringLiteral("accuracyPercent"), event.accuracyPercent);
    setField(QStringLiteral("averageReactionTimeMs"),
             event.averageReactionTimeMs);
    setField(QStringLiteral("shotsPerSecond"), event.shotsPerSecond);

    if (changed) {
        emit realtimeStatsChanged();
    }
}

void UiViewModel::UpdatePerformanceStats(
    const core::events::PerformanceStatsEvent& event) {
    m_performanceStats = {
        {"windowSeconds", event.windowSeconds},
        {"sampledFrames", static_cast<qlonglong>(event.sampledFrames)},
        {"fps", event.fps},
        {"averageFrameTimeMs", event.averageFrameTimeMs},
        {"p95FrameTimeMs", event.p95FrameTimeMs},
        {"worstFrameTimeMs", event.worstFrameTimeMs},
        {"frameJitterMs", event.frameJitterMs},
        {"simulationLoadPercent", event.simulationLoadPercent},
        {"missedTargetFramePercent", event.missedTargetFramePercent},
    };

    emit performanceStatsChanged();
}

void UiViewModel::OnSessionStarted(
    const core::events::SessionStartedEvent& event) {
    m_sessionActive = true;
    m_activeModeId = QString::fromStdString(event.modeId);

    if (m_selectedModeId != m_activeModeId) {
        m_selectedModeId = m_activeModeId;
        emit selectedModeChanged();
    }

    emit sessionStateChanged();
}

void UiViewModel::OnSessionStopped(
    const core::events::SessionStoppedEvent& event) {
    m_sessionActive = false;
    m_activeModeId.clear();

    const std::int64_t stoppedAtMilliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            event.stoppedAt.time_since_epoch())
            .count();
    const QString stoppedAtLocal =
        QDateTime::fromMSecsSinceEpoch(stoppedAtMilliseconds, QTimeZone::UTC)
            .toLocalTime()
            .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    const double elapsedSeconds =
        event.elapsedSeconds > 0.0
            ? event.elapsedSeconds
            : m_realtimeStats.value(QStringLiteral("elapsedSeconds")).toDouble();
    const double configuredDurationSeconds = event.configuredDurationSeconds > 0.0
                                                 ? event.configuredDurationSeconds
                                                 : m_modeDurationSeconds;
    const bool wasSaved =
        event.stopReason != core::events::SessionStopReason::AbortedByUser;
    const QString saveState =
        wasSaved ? QStringLiteral("pending") : QStringLiteral("not_saved");

    m_latestResult = {
        {"sessionId", static_cast<qlonglong>(event.sessionId)},
        {"modeId", QString::fromStdString(event.modeId)},
        {"stoppedAt", stoppedAtLocal},
        {"elapsedSeconds", elapsedSeconds},
        {"configuredDurationSeconds", configuredDurationSeconds},
        {"stopReason",
         event.stopReason == core::events::SessionStopReason::AbortedByUser
             ? QStringLiteral("aborted")
             : QStringLiteral("completed")},
        {"wasSaved", wasSaved},
        {"saveState", saveState},
        {"invalidReasonCode", QStringLiteral("")},
        {"score", m_realtimeStats.value(QStringLiteral("score"))},
        {"hits", m_realtimeStats.value(QStringLiteral("hits"))},
        {"misses", m_realtimeStats.value(QStringLiteral("misses"))},
        {"shotsFired", m_realtimeStats.value(QStringLiteral("shotsFired"))},
        {"accuracyPercent",
         m_realtimeStats.value(QStringLiteral("accuracyPercent"))},
        {"averageReactionTimeMs",
         m_realtimeStats.value(QStringLiteral("averageReactionTimeMs"))},
        {"shotsPerSecond",
         m_realtimeStats.value(QStringLiteral("shotsPerSecond"))},
        {"performanceTier",
         wasSaved ? QStringLiteral("pending") : QStringLiteral("unavailable")},
        {"modeSessionCount", static_cast<qlonglong>(0)},
        {"performanceComparisons", QVariantList{}},
    };

    const bool previousWaiting = m_waitingForSceneClick;
    const int previousCountdown = m_countdownValue;
    const bool previousPauseVisible = m_pauseMenuVisible;
    const bool previousCrosshairVisible = m_crosshairVisible;

    m_waitingForSceneClick = false;
    m_countdownValue = 0;
    m_pauseMenuVisible = false;
    m_crosshairVisible = false;

    m_realtimeStats.insert("modeId", QString::fromStdString(event.modeId));

    emit sessionStateChanged();
    emit realtimeStatsChanged();
    emit latestResultChanged();

    if (previousWaiting != m_waitingForSceneClick ||
        previousCountdown != m_countdownValue ||
        previousPauseVisible != m_pauseMenuVisible ||
        previousCrosshairVisible != m_crosshairVisible) {
        emit trainingOverlayChanged();
    }
}

void UiViewModel::SetLatestResultPerformanceBenchmark(
    std::uint64_t sessionId, const std::string& overallCategory,
    std::uint64_t modeSessionCount,
    const std::vector<PerformanceMetricComparison>& metricComparisons) {
    if (m_latestResult.isEmpty()) {
        return;
    }

    if (!m_latestResult.value(QStringLiteral("wasSaved")).toBool()) {
        return;
    }

    const auto latestSessionId = static_cast<std::uint64_t>(
        m_latestResult.value(QStringLiteral("sessionId")).toLongLong());
    if (latestSessionId == 0 || latestSessionId != sessionId) {
        return;
    }

    bool changed = false;
    const auto setField = [&](const QString& key, const QVariant& value) {
        if (m_latestResult.value(key) == value) {
            return;
        }

        m_latestResult.insert(key, value);
        changed = true;
    };

    QVariantList comparisons;
    comparisons.reserve(static_cast<qsizetype>(metricComparisons.size()));
    for (const auto& comparison : metricComparisons) {
        QVariantMap comparisonVariant;
        comparisonVariant.insert(QStringLiteral("metricId"),
                                 QString::fromStdString(comparison.metricId));
        comparisonVariant.insert(QStringLiteral("tier"),
                                 QString::fromStdString(comparison.tier));
        comparisonVariant.insert(QStringLiteral("value"), comparison.value);
        comparisonVariant.insert(QStringLiteral("average"), comparison.average);
        comparisonVariant.insert(QStringLiteral("best"), comparison.best);
        comparisonVariant.insert(QStringLiteral("worst"), comparison.worst);
        comparisons.push_back(comparisonVariant);
    }

    setField(QStringLiteral("performanceTier"),
             QString::fromStdString(overallCategory));
    setField(QStringLiteral("wasSaved"), true);
    setField(QStringLiteral("saveState"), QStringLiteral("saved"));
    setField(QStringLiteral("invalidReasonCode"), QStringLiteral(""));
    setField(QStringLiteral("modeSessionCount"),
             static_cast<qlonglong>(modeSessionCount));
    setField(QStringLiteral("performanceComparisons"), comparisons);

    if (changed) {
        emit latestResultChanged();
    }
}

void UiViewModel::SetLatestResultInvalid(std::uint64_t sessionId,
                                         const std::string& reasonCode) {
    if (m_latestResult.isEmpty()) {
        return;
    }

    if (m_latestResult.value(QStringLiteral("saveState")).toString() ==
        QStringLiteral("not_saved")) {
        return;
    }

    const auto latestSessionId = static_cast<std::uint64_t>(
        m_latestResult.value(QStringLiteral("sessionId")).toLongLong());
    if (latestSessionId == 0 || latestSessionId != sessionId) {
        return;
    }

    bool changed = false;
    const auto setField = [&](const QString& key, const QVariant& value) {
        if (m_latestResult.value(key) == value) {
            return;
        }

        m_latestResult.insert(key, value);
        changed = true;
    };

    setField(QStringLiteral("wasSaved"), false);
    setField(QStringLiteral("saveState"), QStringLiteral("invalid"));
    setField(QStringLiteral("invalidReasonCode"),
             QString::fromStdString(reasonCode));
    setField(QStringLiteral("performanceTier"), QStringLiteral("unavailable"));
    setField(QStringLiteral("modeSessionCount"), static_cast<qlonglong>(0));
    setField(QStringLiteral("performanceComparisons"), QVariantList{});

    if (changed) {
        emit latestResultChanged();
    }
}

const persistence::AppSettings& UiViewModel::GetSettings() const noexcept {
    return m_settings;
}

std::array<double, 3> UiViewModel::GetViewportBackgroundColor() const noexcept {
    return m_viewportBackgroundColor;
}

std::string UiViewModel::GetSelectedModeIdStd() const {
    return m_selectedModeId.toStdString();
}

QVariantMap UiViewModel::GetTheme() const { return m_theme; }

QVariantMap UiViewModel::GetI18n() const { return m_i18n; }

QVariantList UiViewModel::GetModes() const { return m_modeList; }

QVariantList UiViewModel::GetModeSettings() const { return m_modeSettingsList; }

QVariantList UiViewModel::GetRecentSessions() const { return m_recentSessions; }

QVariantMap UiViewModel::GetBestSession() const { return m_bestSession; }

QVariantMap UiViewModel::GetRealtimeStats() const { return m_realtimeStats; }

QVariantMap UiViewModel::GetPerformanceStats() const {
    return m_performanceStats;
}

QVariantMap UiViewModel::GetLatestResult() const { return m_latestResult; }

QString UiViewModel::GetCurrentScreen() const { return m_currentScreen; }

void UiViewModel::SetCurrentScreen(const QString& screen) {
    if (screen == m_currentScreen || !isSupportedScreen(screen)) {
        return;
    }

    m_currentScreen = screen;
    emit currentScreenChanged();
}

QString UiViewModel::GetSelectedModeId() const { return m_selectedModeId; }

void UiViewModel::SetSelectedModeId(const QString& modeId) {
    if (modeId.isEmpty() || modeId == m_selectedModeId) {
        return;
    }

    m_selectedModeId = modeId;
    const double previousModeDurationSeconds = m_modeDurationSeconds;
    const double previousModeDistanceUnits = m_modeTargetDistance;
    const int previousModeGridRows = m_modeGridRows;
    const int previousModeGridColumns = m_modeGridColumns;
    const int previousModeActiveTargetCount = m_modeActiveTargetCount;
    const bool previousGridConfigAvailable = GetModeGridConfigAvailable();
    const int previousGridCellCount = GetModeGridCellCount();
    const int previousActiveTargetMax = GetModeActiveTargetMax();
    m_modeDurationSeconds =
        std::clamp(_SelectedModeDefaultDurationSeconds(), 10.0, 600.0);
    m_modeTargetDistance =
        std::clamp(_SelectedModeDefaultDistanceUnits(), 1.0, 150.0);

    if (_SelectedModeSupportsGridConfig()) {
        m_modeGridRows = std::clamp(_SelectedModeDefaultGridRows(), 2, 15);
        m_modeGridColumns = std::clamp(_SelectedModeDefaultGridColumns(), 2, 15);
        m_modeActiveTargetCount = _ClampedActiveTargetCountForGrid(
            _SelectedModeDefaultActiveTargetCount(), m_modeGridRows,
            m_modeGridColumns);
    } else {
        m_modeGridRows = 0;
        m_modeGridColumns = 0;
        m_modeActiveTargetCount = 0;
    }

    _ResetModeSettingsForSelection();
    _ApplySelectedModePreferences();
    _RebuildModeSettingsVariant();

    emit selectedModeChanged();
    emit modeSettingsChanged();

    if (!_NearlyEqual(previousModeDurationSeconds, m_modeDurationSeconds)) {
        emit modeDurationChanged();
    }

    if (!_NearlyEqual(previousModeDistanceUnits, m_modeTargetDistance)) {
        emit modeTargetDistanceChanged();
    }

    if (previousModeGridRows != m_modeGridRows) {
        emit modeGridRowsChanged();
    }

    if (previousModeGridColumns != m_modeGridColumns) {
        emit modeGridColumnsChanged();
    }

    if (previousModeActiveTargetCount != m_modeActiveTargetCount) {
        emit modeActiveTargetCountChanged();
    }

    if (previousGridConfigAvailable != GetModeGridConfigAvailable() ||
        previousGridCellCount != GetModeGridCellCount() ||
        previousActiveTargetMax != GetModeActiveTargetMax()) {
        emit modeGridConfigChanged();
    }

    if (m_modeSelectionChangedCallback) {
        m_modeSelectionChangedCallback(GetSelectedModeIdStd());
    }

    const std::string selectedModeIdString = GetSelectedModeIdStd();
    if (m_settings.gameplay.selectedModeId != selectedModeIdString) {
        m_settings.gameplay.selectedModeId = selectedModeIdString;
        _EmitSettingsApplied();
    }
}

double UiViewModel::GetModeDurationSeconds() const {
    return m_modeDurationSeconds;
}

void UiViewModel::SetModeDurationSeconds(double value) {
    const double clamped = std::clamp(value, 10.0, 600.0);
    if (_NearlyEqual(clamped, m_modeDurationSeconds)) {
        return;
    }

    m_modeDurationSeconds = clamped;
    emit modeDurationChanged();

    _PersistSelectedModePreferences();
    _EmitSettingsApplied();
}

double UiViewModel::GetModeTargetDistance() const {
    return m_modeTargetDistance;
}

void UiViewModel::SetModeTargetDistance(double value) {
    const double clamped = std::clamp(value, 1.0, 150.0);
    if (_NearlyEqual(clamped, m_modeTargetDistance)) {
        return;
    }

    m_modeTargetDistance = clamped;
    emit modeTargetDistanceChanged();

    _PersistSelectedModePreferences();
    _EmitSettingsApplied();
}

int UiViewModel::GetModeGridRows() const { return m_modeGridRows; }

void UiViewModel::SetModeGridRows(int value) {
    if (!_SelectedModeSupportsGridConfig()) {
        return;
    }

    const int clampedRows = std::clamp(value, 2, 15);
    if (clampedRows == m_modeGridRows) {
        return;
    }

    const int previousGridCellCount = GetModeGridCellCount();
    const int previousActiveTargetMax = GetModeActiveTargetMax();
    const int previousActiveTargetCount = m_modeActiveTargetCount;

    m_modeGridRows = clampedRows;
    m_modeSettingValues["grid_rows"] = static_cast<double>(m_modeGridRows);
    m_modeActiveTargetCount = _ClampedActiveTargetCountForGrid(
        m_modeActiveTargetCount, m_modeGridRows, m_modeGridColumns);
    m_modeSettingValues["active_target_count"] =
        static_cast<double>(m_modeActiveTargetCount);

    emit modeGridRowsChanged();

    if (previousGridCellCount != GetModeGridCellCount() ||
        previousActiveTargetMax != GetModeActiveTargetMax()) {
        emit modeGridConfigChanged();
    }

    if (previousActiveTargetCount != m_modeActiveTargetCount) {
        emit modeActiveTargetCountChanged();
    }

    _RebuildModeSettingsVariant();
    emit modeSettingsChanged();

    _PersistSelectedModePreferences();
    _EmitSettingsApplied();
}

int UiViewModel::GetModeGridColumns() const { return m_modeGridColumns; }

void UiViewModel::SetModeGridColumns(int value) {
    if (!_SelectedModeSupportsGridConfig()) {
        return;
    }

    const int clampedColumns = std::clamp(value, 2, 15);
    if (clampedColumns == m_modeGridColumns) {
        return;
    }

    const int previousGridCellCount = GetModeGridCellCount();
    const int previousActiveTargetMax = GetModeActiveTargetMax();
    const int previousActiveTargetCount = m_modeActiveTargetCount;

    m_modeGridColumns = clampedColumns;
    m_modeSettingValues["grid_columns"] = static_cast<double>(m_modeGridColumns);
    m_modeActiveTargetCount = _ClampedActiveTargetCountForGrid(
        m_modeActiveTargetCount, m_modeGridRows, m_modeGridColumns);
    m_modeSettingValues["active_target_count"] =
        static_cast<double>(m_modeActiveTargetCount);

    emit modeGridColumnsChanged();

    if (previousGridCellCount != GetModeGridCellCount() ||
        previousActiveTargetMax != GetModeActiveTargetMax()) {
        emit modeGridConfigChanged();
    }

    if (previousActiveTargetCount != m_modeActiveTargetCount) {
        emit modeActiveTargetCountChanged();
    }

    _RebuildModeSettingsVariant();
    emit modeSettingsChanged();

    _PersistSelectedModePreferences();
    _EmitSettingsApplied();
}

int UiViewModel::GetModeActiveTargetCount() const {
    return m_modeActiveTargetCount;
}

void UiViewModel::SetModeActiveTargetCount(int value) {
    if (!_SelectedModeSupportsGridConfig()) {
        return;
    }

    const int clamped = _ClampedActiveTargetCountForGrid(value, m_modeGridRows,
                                                         m_modeGridColumns);
    if (clamped == m_modeActiveTargetCount) {
        return;
    }

    m_modeActiveTargetCount = clamped;
    m_modeSettingValues["active_target_count"] =
        static_cast<double>(m_modeActiveTargetCount);
    emit modeActiveTargetCountChanged();
    _RebuildModeSettingsVariant();
    emit modeSettingsChanged();

    _PersistSelectedModePreferences();
    _EmitSettingsApplied();
}

bool UiViewModel::GetModeGridConfigAvailable() const {
    return _SelectedModeSupportsGridConfig();
}

int UiViewModel::GetModeGridCellCount() const {
    if (!_SelectedModeSupportsGridConfig()) {
        return 0;
    }

    return m_modeGridRows * m_modeGridColumns;
}

int UiViewModel::GetModeActiveTargetMax() const {
    const int gridCellCount = GetModeGridCellCount();
    if (gridCellCount <= 1) {
        return 1;
    }

    return gridCellCount - 1;
}

bool UiViewModel::GetSessionActive() const { return m_sessionActive; }

QString UiViewModel::GetActiveModeId() const { return m_activeModeId; }

bool UiViewModel::GetSceneVisible() const { return m_sceneVisible; }

bool UiViewModel::GetWaitingForSceneClick() const {
    return m_waitingForSceneClick;
}

bool UiViewModel::GetCountdownVisible() const { return m_countdownValue > 0; }

int UiViewModel::GetCountdownValue() const { return m_countdownValue; }

bool UiViewModel::GetPauseMenuVisible() const { return m_pauseMenuVisible; }

bool UiViewModel::GetCrosshairVisible() const { return m_crosshairVisible; }

double UiViewModel::GetCurrentFps() const { return m_currentFps; }

bool UiViewModel::GetFpsCounterEnabled() const {
    return m_settings.ui.fpsCounter.enabled;
}

void UiViewModel::SetFpsCounterEnabled(bool enabled) {
    if (m_settings.ui.fpsCounter.enabled == enabled) {
        return;
    }

    m_settings.ui.fpsCounter.enabled = enabled;
    m_fpsAccumulatorSeconds = 0.0;
    m_fpsAccumulatorFrames = 0;

    if (!enabled && !_NearlyEqual(m_currentFps, 0.0)) {
        m_currentFps = 0.0;
        emit fpsChanged();
    }

    emit settingsChanged();
    _EmitSettingsApplied();
}

QString UiViewModel::GetFpsCounterPosition() const {
    return _ToOverlayAnchorCode(m_settings.ui.fpsCounter.anchor);
}

void UiViewModel::SetFpsCounterPosition(const QString& position) {
    const auto anchor = _ParseOverlayAnchorCode(position);
    if (m_settings.ui.fpsCounter.anchor == anchor) {
        return;
    }

    m_settings.ui.fpsCounter.anchor = anchor;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetCrosshairThickness() const {
    return m_settings.ui.crosshair.thickness;
}

void UiViewModel::SetCrosshairThickness(double value) {
    const double clamped = std::clamp(value, 1.0, 12.0);
    if (_NearlyEqual(clamped, m_settings.ui.crosshair.thickness)) {
        return;
    }

    m_settings.ui.crosshair.thickness = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

bool UiViewModel::GetCrosshairCenterDotEnabled() const {
    return m_settings.ui.crosshair.centerDotEnabled;
}

void UiViewModel::SetCrosshairCenterDotEnabled(bool enabled) {
    if (m_settings.ui.crosshair.centerDotEnabled == enabled) {
        return;
    }

    m_settings.ui.crosshair.centerDotEnabled = enabled;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetCrosshairCenterDotSize() const {
    return m_settings.ui.crosshair.centerDotSize;
}

void UiViewModel::SetCrosshairCenterDotSize(double value) {
    const double clamped = std::clamp(value, 1.0, 16.0);
    if (_NearlyEqual(clamped, m_settings.ui.crosshair.centerDotSize)) {
        return;
    }

    m_settings.ui.crosshair.centerDotSize = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetCrosshairColorRed() const {
    return m_settings.ui.crosshair.color.red;
}

void UiViewModel::SetCrosshairColorRed(double value) {
    const double clamped = _Clamp01(value);
    if (_NearlyEqual(clamped, m_settings.ui.crosshair.color.red)) {
        return;
    }

    m_settings.ui.crosshair.color.red = clamped;
    _RebuildTheme();
    emit settingsChanged();
    emit themeChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetCrosshairColorGreen() const {
    return m_settings.ui.crosshair.color.green;
}

void UiViewModel::SetCrosshairColorGreen(double value) {
    const double clamped = _Clamp01(value);
    if (_NearlyEqual(clamped, m_settings.ui.crosshair.color.green)) {
        return;
    }

    m_settings.ui.crosshair.color.green = clamped;
    _RebuildTheme();
    emit settingsChanged();
    emit themeChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetCrosshairColorBlue() const {
    return m_settings.ui.crosshair.color.blue;
}

void UiViewModel::SetCrosshairColorBlue(double value) {
    const double clamped = _Clamp01(value);
    if (_NearlyEqual(clamped, m_settings.ui.crosshair.color.blue)) {
        return;
    }

    m_settings.ui.crosshair.color.blue = clamped;
    _RebuildTheme();
    emit settingsChanged();
    emit themeChanged();
    _EmitSettingsApplied();
}

bool UiViewModel::GetCrosshairLinesEnabled() const {
    return m_settings.ui.crosshair.linesEnabled;
}

void UiViewModel::SetCrosshairLinesEnabled(bool enabled) {
    if (m_settings.ui.crosshair.linesEnabled == enabled) {
        return;
    }

    m_settings.ui.crosshair.linesEnabled = enabled;
    emit settingsChanged();
    _EmitSettingsApplied();
}

bool UiViewModel::GetCrosshairBorderEnabled() const {
    return m_settings.ui.crosshair.borderEnabled;
}

void UiViewModel::SetCrosshairBorderEnabled(bool enabled) {
    if (m_settings.ui.crosshair.borderEnabled == enabled) {
        return;
    }

    m_settings.ui.crosshair.borderEnabled = enabled;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetCrosshairBorderThickness() const {
    return m_settings.ui.crosshair.borderThickness;
}

void UiViewModel::SetCrosshairBorderThickness(double value) {
    const double clamped = std::clamp(value, 1.0, 6.0);
    if (_NearlyEqual(clamped, m_settings.ui.crosshair.borderThickness)) {
        return;
    }

    m_settings.ui.crosshair.borderThickness = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetCrosshairHorizontalLength() const {
    return m_settings.ui.crosshair.horizontalLength;
}

void UiViewModel::SetCrosshairHorizontalLength(double value) {
    const double clamped = std::clamp(value, 2.0, 40.0);
    if (_NearlyEqual(clamped, m_settings.ui.crosshair.horizontalLength)) {
        return;
    }

    m_settings.ui.crosshair.horizontalLength = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetCrosshairVerticalLength() const {
    return m_settings.ui.crosshair.verticalLength;
}

void UiViewModel::SetCrosshairVerticalLength(double value) {
    const double clamped = std::clamp(value, 2.0, 40.0);
    if (_NearlyEqual(clamped, m_settings.ui.crosshair.verticalLength)) {
        return;
    }

    m_settings.ui.crosshair.verticalLength = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetCrosshairGap() const {
    return m_settings.ui.crosshair.gap;
}

void UiViewModel::SetCrosshairGap(double value) {
    const double clamped = std::clamp(value, 0.0, 20.0);
    if (_NearlyEqual(clamped, m_settings.ui.crosshair.gap)) {
        return;
    }

    m_settings.ui.crosshair.gap = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

bool UiViewModel::GetRawInputEnabled() const {
    return m_settings.input.rawInputEnabled;
}

void UiViewModel::SetRawInputEnabled(bool enabled) {
    if (m_settings.input.rawInputEnabled == enabled) {
        return;
    }

    m_settings.input.rawInputEnabled = enabled;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetCmPer360() const {
    return m_settings.input.sensitivity.cmPer360;
}

void UiViewModel::SetCmPer360(double value) {
    const double clamped = std::clamp(value, 0.1, 200.0);
    if (_NearlyEqual(clamped, m_settings.input.sensitivity.cmPer360)) {
        return;
    }

    m_settings.input.sensitivity.cmPer360 = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetDpi() const { return m_settings.input.sensitivity.dpi; }

void UiViewModel::SetDpi(double value) {
    const double clamped = std::clamp(value, 1.0, 20000.0);
    if (_NearlyEqual(clamped, m_settings.input.sensitivity.dpi)) {
        return;
    }

    m_settings.input.sensitivity.dpi = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetSensitivityScale() const {
    return m_settings.input.sensitivity.sensitivityScale;
}

void UiViewModel::SetSensitivityScale(double value) {
    const double clamped = std::clamp(value, 0.01, 5.0);
    if (_NearlyEqual(clamped, m_settings.input.sensitivity.sensitivityScale)) {
        return;
    }

    m_settings.input.sensitivity.sensitivityScale = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetYawMultiplier() const {
    return m_settings.input.sensitivity.yawMultiplier;
}

void UiViewModel::SetYawMultiplier(double value) {
    const double clamped = std::clamp(value, 0.01, 5.0);
    if (_NearlyEqual(clamped, m_settings.input.sensitivity.yawMultiplier)) {
        return;
    }

    m_settings.input.sensitivity.yawMultiplier = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetPitchMultiplier() const {
    return m_settings.input.sensitivity.pitchMultiplier;
}

void UiViewModel::SetPitchMultiplier(double value) {
    const double clamped = std::clamp(value, 0.01, 5.0);
    if (_NearlyEqual(clamped, m_settings.input.sensitivity.pitchMultiplier)) {
        return;
    }

    m_settings.input.sensitivity.pitchMultiplier = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

bool UiViewModel::GetInvertY() const {
    return m_settings.input.sensitivity.invertY;
}

void UiViewModel::SetInvertY(bool enabled) {
    if (m_settings.input.sensitivity.invertY == enabled) {
        return;
    }

    m_settings.input.sensitivity.invertY = enabled;
    emit settingsChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetTargetColorRed() const {
    return m_settings.gameplay.target.color.red;
}

void UiViewModel::SetTargetColorRed(double value) {
    const double clamped = _Clamp01(value);
    if (_NearlyEqual(clamped, m_settings.gameplay.target.color.red)) {
        return;
    }

    m_settings.gameplay.target.color.red = clamped;
    _RebuildTheme();

    emit settingsChanged();
    emit themeChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetTargetColorGreen() const {
    return m_settings.gameplay.target.color.green;
}

void UiViewModel::SetTargetColorGreen(double value) {
    const double clamped = _Clamp01(value);
    if (_NearlyEqual(clamped, m_settings.gameplay.target.color.green)) {
        return;
    }

    m_settings.gameplay.target.color.green = clamped;
    _RebuildTheme();

    emit settingsChanged();
    emit themeChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetTargetColorBlue() const {
    return m_settings.gameplay.target.color.blue;
}

void UiViewModel::SetTargetColorBlue(double value) {
    const double clamped = _Clamp01(value);
    if (_NearlyEqual(clamped, m_settings.gameplay.target.color.blue)) {
        return;
    }

    m_settings.gameplay.target.color.blue = clamped;
    _RebuildTheme();

    emit settingsChanged();
    emit themeChanged();
    _EmitSettingsApplied();
}

double UiViewModel::GetTargetRadius() const {
    return m_settings.gameplay.target.radius;
}

void UiViewModel::SetTargetRadius(double value) {
    const double clamped = std::clamp(value, 0.05, 5.0);
    if (_NearlyEqual(clamped, m_settings.gameplay.target.radius)) {
        return;
    }

    m_settings.gameplay.target.radius = clamped;
    emit settingsChanged();
    _EmitSettingsApplied();
}

QString UiViewModel::GetThemeMode() const {
    return _ToThemeModeString(m_settings.ui.themeMode);
}

void UiViewModel::SetThemeMode(const QString& mode) {
    const auto parsedThemeMode = _ParseThemeMode(mode);
    if (parsedThemeMode == m_settings.ui.themeMode) {
        return;
    }

    m_settings.ui.themeMode = parsedThemeMode;
    _RebuildTheme();

    emit settingsChanged();
    emit themeChanged();
    _EmitSettingsApplied();
}

QString UiViewModel::GetLanguageCode() const {
    return _ToLanguageCodeString(m_settings.ui.language);
}

void UiViewModel::SetLanguageCode(const QString& code) {
    const auto parsedLanguage = _ParseLanguageCode(code);
    if (parsedLanguage == m_settings.ui.language) {
        return;
    }

    m_settings.ui.language = parsedLanguage;
    _RebuildI18n();
    _RebuildModesVariant();
    _RebuildModeSettingsVariant();

    emit settingsChanged();
    emit translationsChanged();
    emit modesChanged();
    emit modeSettingsChanged();
    _EmitSettingsApplied();
}

bool UiViewModel::GetStartFullscreen() const {
    return m_settings.window.startFullscreen;
}

void UiViewModel::SetStartFullscreen(bool value) {
    if (m_settings.window.startFullscreen == value) {
        return;
    }

    m_settings.window.startFullscreen = value;
    emit settingsChanged();
    _EmitSettingsApplied();
}

QString UiViewModel::GetKeybindToggleFullscreen() const {
    return QString::fromStdString(m_settings.keybindings.toggleFullscreen);
}

void UiViewModel::_SetKeybindField(std::string& target, const QString& key) {
    const std::string keyStr = key.trimmed().toStdString();
    if (keyStr.empty() || keyStr == target) return;
    target = keyStr;
    emit settingsChanged();
    _EmitSettingsApplied();
}

void UiViewModel::SetKeybindToggleFullscreen(const QString& key) {
    _SetKeybindField(m_settings.keybindings.toggleFullscreen, key);
}

QString UiViewModel::GetKeybindToggleFpsCounter() const {
    return QString::fromStdString(m_settings.keybindings.toggleFpsCounter);
}

void UiViewModel::SetKeybindToggleFpsCounter(const QString& key) {
    _SetKeybindField(m_settings.keybindings.toggleFpsCounter, key);
}

QString UiViewModel::GetKeybindToggleCrosshair() const {
    return QString::fromStdString(m_settings.keybindings.toggleCrosshair);
}

void UiViewModel::SetKeybindToggleCrosshair(const QString& key) {
    _SetKeybindField(m_settings.keybindings.toggleCrosshair, key);
}

void UiViewModel::ToggleFpsCounterRuntime() {
    m_settings.ui.fpsCounterEnabled = !m_settings.ui.fpsCounterEnabled;
    emit settingsChanged();
}

bool UiViewModel::GetCrosshairKeyToggleEnabled() const {
    return m_crosshairKeyToggle;
}

void UiViewModel::ToggleCrosshairKeyOverride() {
    m_crosshairKeyToggle = !m_crosshairKeyToggle;
    emit trainingOverlayChanged();
}

void UiViewModel::RequestStartSelectedMode() {
    SetCurrentScreen(QStringLiteral("training"));

    if (m_startModeCallback) {
        m_startModeCallback(GetSelectedModeIdStd(), m_modeDurationSeconds,
                            m_modeTargetDistance, m_modeSettingValues);
    }
}

void UiViewModel::SetModeSettingValue(const QString& settingKey, double value) {
    const ModeDescriptor* mode = _SelectedModeDescriptor();
    if (mode == nullptr) {
        return;
    }

    const std::string key = settingKey.toStdString();
    const auto settingIterator = std::find_if(
        mode->settings.begin(), mode->settings.end(),
        [&](const ModeSettingDescriptor& setting) { return setting.key == key; });

    if (settingIterator == mode->settings.end()) {
        return;
    }

    const double low =
        std::min(settingIterator->minValue, settingIterator->maxValue);
    const double high =
        std::max(settingIterator->minValue, settingIterator->maxValue);

    double clampedValue = std::clamp(value, low, high);
    if (settingIterator->integerOnly) {
        clampedValue = std::round(clampedValue);
    }

    m_modeSettingValues[key] = clampedValue;

    if (key == "grid_rows") {
        m_modeGridRows = static_cast<int>(clampedValue);
    } else if (key == "grid_columns") {
        m_modeGridColumns = static_cast<int>(clampedValue);
    } else if (key == "active_target_count") {
        m_modeActiveTargetCount = static_cast<int>(clampedValue);
    }

    if (_SelectedModeSupportsGridConfig()) {
        const int clampedActiveCount = _ClampedActiveTargetCountForGrid(
            m_modeActiveTargetCount, m_modeGridRows, m_modeGridColumns);
        m_modeActiveTargetCount = clampedActiveCount;
        m_modeSettingValues["active_target_count"] =
            static_cast<double>(clampedActiveCount);
    }

    _RebuildModeSettingsVariant();
    emit modeSettingsChanged();

    _PersistSelectedModePreferences();
    _EmitSettingsApplied();
}

void UiViewModel::ResetSelectedModeSettingsToDefaults() {
    const ModeDescriptor* mode = _SelectedModeDescriptor();
    if (mode == nullptr) {
        return;
    }

    const double previousModeDurationSeconds = m_modeDurationSeconds;
    const double previousModeDistanceUnits = m_modeTargetDistance;
    const int previousModeGridRows = m_modeGridRows;
    const int previousModeGridColumns = m_modeGridColumns;
    const int previousModeActiveTargetCount = m_modeActiveTargetCount;
    const bool previousGridConfigAvailable = GetModeGridConfigAvailable();
    const int previousGridCellCount = GetModeGridCellCount();
    const int previousActiveTargetMax = GetModeActiveTargetMax();

    m_modeDurationSeconds =
        std::clamp(_SelectedModeDefaultDurationSeconds(), 10.0, 600.0);
    m_modeTargetDistance =
        std::clamp(_SelectedModeDefaultDistanceUnits(), 1.0, 150.0);

    if (_SelectedModeSupportsGridConfig()) {
        m_modeGridRows = std::clamp(_SelectedModeDefaultGridRows(), 2, 15);
        m_modeGridColumns = std::clamp(_SelectedModeDefaultGridColumns(), 2, 15);
        m_modeActiveTargetCount = _ClampedActiveTargetCountForGrid(
            _SelectedModeDefaultActiveTargetCount(), m_modeGridRows,
            m_modeGridColumns);
    } else {
        m_modeGridRows = 0;
        m_modeGridColumns = 0;
        m_modeActiveTargetCount = 0;
    }

    _ResetModeSettingsForSelection();
    _RebuildModeSettingsVariant();
    emit modeSettingsChanged();

    bool anyStateChanged = false;

    if (!_NearlyEqual(previousModeDurationSeconds, m_modeDurationSeconds)) {
        emit modeDurationChanged();
        anyStateChanged = true;
    }

    if (!_NearlyEqual(previousModeDistanceUnits, m_modeTargetDistance)) {
        emit modeTargetDistanceChanged();
        anyStateChanged = true;
    }

    if (previousModeGridRows != m_modeGridRows) {
        emit modeGridRowsChanged();
        anyStateChanged = true;
    }

    if (previousModeGridColumns != m_modeGridColumns) {
        emit modeGridColumnsChanged();
        anyStateChanged = true;
    }

    if (previousModeActiveTargetCount != m_modeActiveTargetCount) {
        emit modeActiveTargetCountChanged();
        anyStateChanged = true;
    }

    if (previousGridConfigAvailable != GetModeGridConfigAvailable() ||
        previousGridCellCount != GetModeGridCellCount() ||
        previousActiveTargetMax != GetModeActiveTargetMax()) {
        emit modeGridConfigChanged();
        anyStateChanged = true;
    }

    const bool removedPersistedOverride =
        m_settings.gameplay.modeOverrides.erase(mode->id) > 0;

    if (anyStateChanged || removedPersistedOverride) {
        _EmitSettingsApplied();
    }
}

void UiViewModel::RequestStopMode() {
    if (m_exitTrainingCallback) {
        m_exitTrainingCallback();
        return;
    }

    if (m_stopModeCallback) {
        m_stopModeCallback();
    }
}

void UiViewModel::RequestContinueTraining() {
    SetCurrentScreen(QStringLiteral("training"));

    if (m_continueTrainingCallback) {
        m_continueTrainingCallback();
    }
}

void UiViewModel::RequestExitTraining() {
    if (m_exitTrainingCallback) {
        m_exitTrainingCallback();
        return;
    }

    if (m_stopModeCallback) {
        m_stopModeCallback();
    }
}

void UiViewModel::RequestOpenSettings() {
    SetCurrentScreen(QStringLiteral("settings"));
}

void UiViewModel::RequestBackFromSettings() {
    SetCurrentScreen(QStringLiteral("training"));
}

void UiViewModel::ShowTrainingSceneAwaitingTrigger() {
    const bool previousSceneVisible = m_sceneVisible;
    const bool previousWaiting = m_waitingForSceneClick;
    const int previousCountdown = m_countdownValue;
    const bool previousPauseVisible = m_pauseMenuVisible;
    const bool previousCrosshairVisible = m_crosshairVisible;

    m_sceneVisible = true;
    m_waitingForSceneClick = true;
    m_countdownValue = 0;
    m_pauseMenuVisible = false;
    m_crosshairVisible = false;

    SetCurrentScreen(QStringLiteral("training"));

    if (previousSceneVisible != m_sceneVisible ||
        previousWaiting != m_waitingForSceneClick ||
        previousCountdown != m_countdownValue ||
        previousPauseVisible != m_pauseMenuVisible ||
        previousCrosshairVisible != m_crosshairVisible) {
        emit trainingOverlayChanged();
    }
}

void UiViewModel::ShowTrainingCountdown(int secondsRemaining) {
    const int clamped = std::max(0, secondsRemaining);

    const bool previousSceneVisible = m_sceneVisible;
    const bool previousWaiting = m_waitingForSceneClick;
    const int previousCountdown = m_countdownValue;
    const bool previousPauseVisible = m_pauseMenuVisible;
    const bool previousCrosshairVisible = m_crosshairVisible;

    m_sceneVisible = true;
    m_waitingForSceneClick = false;
    m_countdownValue = clamped;
    m_pauseMenuVisible = false;
    m_crosshairVisible = false;

    SetCurrentScreen(QStringLiteral("training"));

    if (previousSceneVisible != m_sceneVisible ||
        previousWaiting != m_waitingForSceneClick ||
        previousCountdown != m_countdownValue ||
        previousPauseVisible != m_pauseMenuVisible ||
        previousCrosshairVisible != m_crosshairVisible) {
        emit trainingOverlayChanged();
    }
}

void UiViewModel::ShowTrainingActive() {
    const bool previousSceneVisible = m_sceneVisible;
    const bool previousWaiting = m_waitingForSceneClick;
    const int previousCountdown = m_countdownValue;
    const bool previousPauseVisible = m_pauseMenuVisible;
    const bool previousCrosshairVisible = m_crosshairVisible;

    m_sceneVisible = true;
    m_waitingForSceneClick = false;
    m_countdownValue = 0;
    m_pauseMenuVisible = false;
    m_crosshairVisible = true;

    SetCurrentScreen(QStringLiteral("training"));

    if (previousSceneVisible != m_sceneVisible ||
        previousWaiting != m_waitingForSceneClick ||
        previousCountdown != m_countdownValue ||
        previousPauseVisible != m_pauseMenuVisible ||
        previousCrosshairVisible != m_crosshairVisible) {
        emit trainingOverlayChanged();
    }
}

void UiViewModel::ShowTrainingPaused() {
    const bool previousSceneVisible = m_sceneVisible;
    const bool previousWaiting = m_waitingForSceneClick;
    const int previousCountdown = m_countdownValue;
    const bool previousPauseVisible = m_pauseMenuVisible;
    const bool previousCrosshairVisible = m_crosshairVisible;

    m_sceneVisible = true;
    m_waitingForSceneClick = false;
    m_countdownValue = 0;
    m_pauseMenuVisible = true;
    m_crosshairVisible = true;

    SetCurrentScreen(QStringLiteral("training"));

    if (previousSceneVisible != m_sceneVisible ||
        previousWaiting != m_waitingForSceneClick ||
        previousCountdown != m_countdownValue ||
        previousPauseVisible != m_pauseMenuVisible ||
        previousCrosshairVisible != m_crosshairVisible) {
        emit trainingOverlayChanged();
    }
}

void UiViewModel::ReturnToMainMenu() {
    const bool previousSceneVisible = m_sceneVisible;
    const bool previousWaiting = m_waitingForSceneClick;
    const int previousCountdown = m_countdownValue;
    const bool previousPauseVisible = m_pauseMenuVisible;
    const bool previousCrosshairVisible = m_crosshairVisible;

    m_sceneVisible = false;
    m_waitingForSceneClick = false;
    m_countdownValue = 0;
    m_pauseMenuVisible = false;
    m_crosshairVisible = false;

    SetCurrentScreen(QStringLiteral("menu"));

    if (previousSceneVisible != m_sceneVisible ||
        previousWaiting != m_waitingForSceneClick ||
        previousCountdown != m_countdownValue ||
        previousPauseVisible != m_pauseMenuVisible ||
        previousCrosshairVisible != m_crosshairVisible) {
        emit trainingOverlayChanged();
    }
}

double UiViewModel::_Clamp01(double value) noexcept {
    return std::clamp(value, 0.0, 1.0);
}

bool UiViewModel::_NearlyEqual(double left, double right) noexcept {
    return std::abs(left - right) <= 0.000001;
}

persistence::UiThemeMode UiViewModel::_ParseThemeMode(const QString& mode) {
    return mode.trimmed().compare(QStringLiteral("light"), Qt::CaseInsensitive) ==
                   0
               ? persistence::UiThemeMode::Light
               : persistence::UiThemeMode::Dark;
}

QString UiViewModel::_ToThemeModeString(persistence::UiThemeMode mode) {
    switch (mode) {
        case persistence::UiThemeMode::Light:
            return QStringLiteral("light");
        case persistence::UiThemeMode::Dark:
        default:
            return QStringLiteral("dark");
    }
}

persistence::UiLanguage UiViewModel::_ParseLanguageCode(const QString& code) {
    return code.trimmed().compare(QStringLiteral("tr"), Qt::CaseInsensitive) == 0
               ? persistence::UiLanguage::Turkish
               : persistence::UiLanguage::English;
}

QString UiViewModel::_ToLanguageCodeString(persistence::UiLanguage language) {
    switch (language) {
        case persistence::UiLanguage::Turkish:
            return QStringLiteral("tr");
        case persistence::UiLanguage::English:
        default:
            return QStringLiteral("en");
    }
}

persistence::UiOverlayAnchor
UiViewModel::_ParseOverlayAnchorCode(const QString& code) {
    const QString normalized = code.trimmed().toLower();

    if (normalized == QStringLiteral("top_left") ||
        normalized == QStringLiteral("topleft")) {
        return persistence::UiOverlayAnchor::TopLeft;
    }

    if (normalized == QStringLiteral("bottom_left") ||
        normalized == QStringLiteral("bottomleft")) {
        return persistence::UiOverlayAnchor::BottomLeft;
    }

    if (normalized == QStringLiteral("bottom_right") ||
        normalized == QStringLiteral("bottomright")) {
        return persistence::UiOverlayAnchor::BottomRight;
    }

    return persistence::UiOverlayAnchor::TopRight;
}

QString UiViewModel::_ToOverlayAnchorCode(persistence::UiOverlayAnchor anchor) {
    switch (anchor) {
        case persistence::UiOverlayAnchor::TopLeft:
            return QStringLiteral("top_left");
        case persistence::UiOverlayAnchor::BottomLeft:
            return QStringLiteral("bottom_left");
        case persistence::UiOverlayAnchor::BottomRight:
            return QStringLiteral("bottom_right");
        case persistence::UiOverlayAnchor::TopRight:
        default:
            return QStringLiteral("top_right");
    }
}

std::array<double, 3> UiViewModel::_Mix(const std::array<double, 3>& left,
                                        const std::array<double, 3>& right,
                                        double factor) {
    const double safeFactor = std::clamp(factor, 0.0, 1.0);
    const double inverse = 1.0 - safeFactor;

    return {
        _Clamp01(left[0] * inverse + right[0] * safeFactor),
        _Clamp01(left[1] * inverse + right[1] * safeFactor),
        _Clamp01(left[2] * inverse + right[2] * safeFactor),
    };
}

double UiViewModel::_Luminance(const std::array<double, 3>& color) noexcept {
    return 0.2126 * color[0] + 0.7152 * color[1] + 0.0722 * color[2];
}

QString UiViewModel::_ToHexColor(const std::array<double, 3>& color) {
    QColor qColor;
    qColor.setRgbF(_Clamp01(color[0]), _Clamp01(color[1]), _Clamp01(color[2]),
                   1.0);
    return qColor.name(QColor::HexRgb);
}

QString UiViewModel::_LocalizedModeName(const ModeDescriptor& mode) const {
    return I18nProvider::ModeName(m_settings.ui.language, mode.id, mode.displayName);
}

QString UiViewModel::_LocalizedModeDescription(const ModeDescriptor& mode) const {
    return I18nProvider::ModeDescription(m_settings.ui.language, mode.id, mode.description);
}

QString UiViewModel::_LocalizedModeSettingName(
    const std::string& modeId, const ModeSettingDescriptor& setting) const {
    return I18nProvider::ModeSettingName(m_settings.ui.language, modeId, setting.key, setting.displayName);
}

const UiViewModel::ModeDescriptor*
UiViewModel::_SelectedModeDescriptor() const {
    const auto iterator = std::find_if(
        m_modes.begin(), m_modes.end(), [&](const ModeDescriptor& mode) {
            return m_selectedModeId == QString::fromStdString(mode.id);
        });

    if (iterator == m_modes.end()) {
        return nullptr;
    }

    return &(*iterator);
}

void UiViewModel::_ResetModeSettingsForSelection() {
    m_modeSettingValues.clear();

    const ModeDescriptor* mode = _SelectedModeDescriptor();
    if (mode == nullptr) {
        return;
    }

    for (const auto& setting : mode->settings) {
        const double low = std::min(setting.minValue, setting.maxValue);
        const double high = std::max(setting.minValue, setting.maxValue);

        double value = std::clamp(setting.defaultValue, low, high);
        if (setting.integerOnly) {
            value = std::round(value);
        }

        m_modeSettingValues.emplace(setting.key, value);
    }

    if (_SelectedModeSupportsGridConfig()) {
        const auto gridRowsIterator = m_modeSettingValues.find("grid_rows");
        const auto gridColumnsIterator = m_modeSettingValues.find("grid_columns");
        const auto activeTargetCountIterator =
            m_modeSettingValues.find("active_target_count");

        if (gridRowsIterator != m_modeSettingValues.end()) {
            m_modeGridRows = static_cast<int>(gridRowsIterator->second);
        }

        if (gridColumnsIterator != m_modeSettingValues.end()) {
            m_modeGridColumns = static_cast<int>(gridColumnsIterator->second);
        }

        if (activeTargetCountIterator != m_modeSettingValues.end()) {
            m_modeActiveTargetCount =
                static_cast<int>(activeTargetCountIterator->second);
        }

        const int clampedActiveCount = _ClampedActiveTargetCountForGrid(
            m_modeActiveTargetCount, m_modeGridRows, m_modeGridColumns);
        m_modeActiveTargetCount = clampedActiveCount;
        m_modeSettingValues["active_target_count"] =
            static_cast<double>(clampedActiveCount);
    }
}

void UiViewModel::_ApplySelectedModePreferences() {
    const ModeDescriptor* mode = _SelectedModeDescriptor();
    if (mode == nullptr) {
        return;
    }

    const auto modePreferencesIterator =
        m_settings.gameplay.modeOverrides.find(mode->id);
    if (modePreferencesIterator == m_settings.gameplay.modeOverrides.end()) {
        return;
    }

    const persistence::ModePreferences& modePreferences =
        modePreferencesIterator->second;

    if (std::isfinite(modePreferences.durationSeconds) &&
        modePreferences.durationSeconds > 0.0) {
        m_modeDurationSeconds =
            std::clamp(modePreferences.durationSeconds, 10.0, 600.0);
    }

    if (std::isfinite(modePreferences.distanceUnits) &&
        modePreferences.distanceUnits > 0.0) {
        m_modeTargetDistance =
            std::clamp(modePreferences.distanceUnits, 1.0, 150.0);
    }

    for (const auto& setting : mode->settings) {
        const auto persistedSettingIterator =
            modePreferences.settingValues.find(setting.key);
        if (persistedSettingIterator == modePreferences.settingValues.end()) {
            continue;
        }

        const double low = std::min(setting.minValue, setting.maxValue);
        const double high = std::max(setting.minValue, setting.maxValue);
        double value = persistedSettingIterator->second;

        if (!std::isfinite(value)) {
            continue;
        }

        value = std::clamp(value, low, high);
        if (setting.integerOnly) {
            value = std::round(value);
        }

        m_modeSettingValues[setting.key] = value;
    }

    if (_SelectedModeSupportsGridConfig()) {
        const auto gridRowsIterator = m_modeSettingValues.find("grid_rows");
        const auto gridColumnsIterator = m_modeSettingValues.find("grid_columns");
        const auto activeTargetCountIterator =
            m_modeSettingValues.find("active_target_count");

        if (gridRowsIterator != m_modeSettingValues.end()) {
            m_modeGridRows = std::clamp(
                static_cast<int>(std::lround(gridRowsIterator->second)), 2, 15);
            m_modeSettingValues["grid_rows"] = static_cast<double>(m_modeGridRows);
        }

        if (gridColumnsIterator != m_modeSettingValues.end()) {
            m_modeGridColumns = std::clamp(
                static_cast<int>(std::lround(gridColumnsIterator->second)), 2, 15);
            m_modeSettingValues["grid_columns"] =
                static_cast<double>(m_modeGridColumns);
        }

        if (activeTargetCountIterator != m_modeSettingValues.end()) {
            m_modeActiveTargetCount =
                static_cast<int>(std::lround(activeTargetCountIterator->second));
        }

        const int clampedActiveCount = _ClampedActiveTargetCountForGrid(
            m_modeActiveTargetCount, m_modeGridRows, m_modeGridColumns);
        m_modeActiveTargetCount = clampedActiveCount;
        m_modeSettingValues["active_target_count"] =
            static_cast<double>(clampedActiveCount);
    }
}

void UiViewModel::_PersistSelectedModePreferences() {
    const ModeDescriptor* mode = _SelectedModeDescriptor();
    if (mode == nullptr) {
        return;
    }

    persistence::ModePreferences modePreferences;
    modePreferences.durationSeconds =
        std::clamp(m_modeDurationSeconds, 10.0, 600.0);
    modePreferences.distanceUnits = std::clamp(m_modeTargetDistance, 1.0, 150.0);

    for (const auto& setting : mode->settings) {
        const auto valueIterator = m_modeSettingValues.find(setting.key);
        double value = valueIterator != m_modeSettingValues.end()
                           ? valueIterator->second
                           : setting.defaultValue;

        const double low = std::min(setting.minValue, setting.maxValue);
        const double high = std::max(setting.minValue, setting.maxValue);
        value = std::clamp(value, low, high);

        if (setting.integerOnly) {
            value = std::round(value);
        }

        modePreferences.settingValues.emplace(setting.key, value);
    }

    if (_SelectedModeSupportsGridConfig()) {
        modePreferences.settingValues["grid_rows"] =
            static_cast<double>(m_modeGridRows);
        modePreferences.settingValues["grid_columns"] =
            static_cast<double>(m_modeGridColumns);
        modePreferences.settingValues["active_target_count"] =
            static_cast<double>(_ClampedActiveTargetCountForGrid(
                m_modeActiveTargetCount, m_modeGridRows, m_modeGridColumns));
    }

    m_settings.gameplay.modeOverrides[mode->id] = std::move(modePreferences);
}

void UiViewModel::_RebuildModeSettingsVariant() {
    QVariantList modeSettings;

    const ModeDescriptor* mode = _SelectedModeDescriptor();
    if (mode == nullptr) {
        m_modeSettingsList = modeSettings;
        return;
    }

    for (const auto& setting : mode->settings) {
        const auto valueIterator = m_modeSettingValues.find(setting.key);
        double currentValue = valueIterator != m_modeSettingValues.end()
                                  ? valueIterator->second
                                  : setting.defaultValue;

        double minValue = setting.minValue;
        double maxValue = setting.maxValue;

        if (mode->id == "gridshot" && setting.key == "active_target_count") {
            const auto rowIterator = m_modeSettingValues.find("grid_rows");
            const auto columnIterator = m_modeSettingValues.find("grid_columns");
            const int rows =
                rowIterator != m_modeSettingValues.end()
                    ? std::max(2, static_cast<int>(std::lround(rowIterator->second)))
                    : std::max(2, _SelectedModeDefaultGridRows());
            const int columns =
                columnIterator != m_modeSettingValues.end()
                    ? std::max(2,
                               static_cast<int>(std::lround(columnIterator->second)))
                    : std::max(2, _SelectedModeDefaultGridColumns());

            maxValue = static_cast<double>(std::max(1, rows * columns - 1));
            currentValue = std::clamp(currentValue, minValue, maxValue);
            m_modeSettingValues[setting.key] = currentValue;
        }

        QVariantMap settingVariant;
        settingVariant.insert("id", QString::fromStdString(setting.key));
        settingVariant.insert("name", _LocalizedModeSettingName(mode->id, setting));
        settingVariant.insert("value", currentValue);
        settingVariant.insert("minValue", minValue);
        settingVariant.insert("maxValue", maxValue);
        settingVariant.insert("step", setting.step);
        settingVariant.insert("integerOnly", setting.integerOnly);

        modeSettings.push_back(settingVariant);
    }

    m_modeSettingsList = modeSettings;
}

double UiViewModel::_SelectedModeDefaultDurationSeconds() const {
    const auto iterator = std::find_if(
        m_modes.begin(), m_modes.end(), [&](const ModeDescriptor& mode) {
            return m_selectedModeId == QString::fromStdString(mode.id);
        });

    if (iterator == m_modes.end()) {
        return 60.0;
    }

    return iterator->defaultDurationSeconds;
}

double UiViewModel::_SelectedModeDefaultDistanceUnits() const {
    const auto iterator = std::find_if(
        m_modes.begin(), m_modes.end(), [&](const ModeDescriptor& mode) {
            return m_selectedModeId == QString::fromStdString(mode.id);
        });

    if (iterator == m_modes.end()) {
        return 25.0;
    }

    return iterator->defaultDistanceUnits;
}

int UiViewModel::_SelectedModeDefaultGridRows() const {
    const auto iterator = std::find_if(
        m_modes.begin(), m_modes.end(), [&](const ModeDescriptor& mode) {
            return m_selectedModeId == QString::fromStdString(mode.id);
        });

    if (iterator == m_modes.end()) {
        return 0;
    }

    return iterator->defaultGridRows;
}

int UiViewModel::_SelectedModeDefaultGridColumns() const {
    const auto iterator = std::find_if(
        m_modes.begin(), m_modes.end(), [&](const ModeDescriptor& mode) {
            return m_selectedModeId == QString::fromStdString(mode.id);
        });

    if (iterator == m_modes.end()) {
        return 0;
    }

    return iterator->defaultGridColumns;
}

int UiViewModel::_SelectedModeDefaultActiveTargetCount() const {
    const auto iterator = std::find_if(
        m_modes.begin(), m_modes.end(), [&](const ModeDescriptor& mode) {
            return m_selectedModeId == QString::fromStdString(mode.id);
        });

    if (iterator == m_modes.end()) {
        return 0;
    }

    return iterator->defaultActiveTargetCount;
}

bool UiViewModel::_SelectedModeSupportsGridConfig() const {
    return _SelectedModeDefaultGridRows() > 0 &&
           _SelectedModeDefaultGridColumns() > 0 &&
           _SelectedModeDefaultActiveTargetCount() > 0;
}

int UiViewModel::_ClampedActiveTargetCountForGrid(int value, int rows,
                                                  int columns) const {
    const int gridCellCount = rows * columns;
    if (gridCellCount <= 1) {
        return 1;
    }

    return std::clamp(value, 1, gridCellCount - 1);
}

void UiViewModel::_RebuildTheme() {
    const std::array<double, 3> targetColor = {
        _Clamp01(m_settings.gameplay.target.color.red),
        _Clamp01(m_settings.gameplay.target.color.green),
        _Clamp01(m_settings.gameplay.target.color.blue),
    };

    const bool darkTheme =
        m_settings.ui.themeMode == persistence::UiThemeMode::Dark;

    std::array<double, 3> windowColor;
    std::array<double, 3> surfaceColor;
    std::array<double, 3> cardColor;
    std::array<double, 3> borderColor;
    std::array<double, 3> textPrimaryColor;
    std::array<double, 3> textSecondaryColor;
    std::array<double, 3> accentColor;
    std::array<double, 3> accentSoftColor;
    std::array<double, 3> dangerColor;
    std::array<double, 3> viewportColor;

    if (darkTheme) {
        windowColor = _Mix(targetColor, {0.02, 0.03, 0.05}, 0.90);
        surfaceColor = _Mix(targetColor, {0.03, 0.05, 0.08}, 0.84);
        cardColor = _Mix(targetColor, {0.05, 0.08, 0.12}, 0.78);
        borderColor = _Mix(surfaceColor, {0.55, 0.58, 0.63}, 0.22);
        accentColor = _Mix(targetColor, {0.98, 0.99, 1.00}, 0.06);
        accentSoftColor = _Mix(targetColor, {0.62, 0.68, 0.78}, 0.36);
        dangerColor = {0.88, 0.33, 0.33};
        textPrimaryColor = {0.94, 0.95, 0.97};
        textSecondaryColor = {0.76, 0.79, 0.84};
        viewportColor = _Mix(targetColor, {0.02, 0.03, 0.05}, 0.82);
    } else {
        windowColor = _Mix(targetColor, {0.98, 0.99, 1.00}, 0.92);
        surfaceColor = _Mix(targetColor, {0.95, 0.97, 0.99}, 0.82);
        cardColor = _Mix(targetColor, {0.92, 0.95, 0.98}, 0.74);
        borderColor = _Mix(surfaceColor, {0.40, 0.45, 0.52}, 0.20);
        accentColor = _Mix(targetColor, {0.14, 0.20, 0.33}, 0.12);
        accentSoftColor = _Mix(targetColor, {0.88, 0.91, 0.95}, 0.42);
        dangerColor = {0.78, 0.21, 0.24};
        textPrimaryColor = {0.10, 0.13, 0.18};
        textSecondaryColor = {0.29, 0.34, 0.41};
        viewportColor = _Mix(targetColor, {0.52, 0.57, 0.64}, 0.35);
    }

    const std::array<double, 3> accentTextColor =
        _Luminance(accentColor) >= 0.62 ? std::array<double, 3>{0.10, 0.12, 0.15}
                                        : std::array<double, 3>{0.96, 0.97, 0.99};
    const std::array<double, 3> crosshairColor = {
        _Clamp01(m_settings.ui.crosshair.color.red),
        _Clamp01(m_settings.ui.crosshair.color.green),
        _Clamp01(m_settings.ui.crosshair.color.blue),
    };

    m_viewportBackgroundColor = viewportColor;

    m_theme = {
        {"isDark", darkTheme},
        {"window", _ToHexColor(windowColor)},
        {"surface", _ToHexColor(surfaceColor)},
        {"card", _ToHexColor(cardColor)},
        {"border", _ToHexColor(borderColor)},
        {"textPrimary", _ToHexColor(textPrimaryColor)},
        {"textSecondary", _ToHexColor(textSecondaryColor)},
        {"accent", _ToHexColor(accentColor)},
        {"accentSoft", _ToHexColor(accentSoftColor)},
        {"accentText", _ToHexColor(accentTextColor)},
        {"danger", _ToHexColor(dangerColor)},
        {"crosshair", _ToHexColor(crosshairColor)},
        {"overlayScrim",
         darkTheme ? QStringLiteral("#88000000") : QStringLiteral("#66000000")},
        {"viewportBackground",
         QVariantList{viewportColor[0], viewportColor[1], viewportColor[2]}},
    };
}

void UiViewModel::_RebuildI18n() {
    m_i18n = I18nProvider::Build(m_settings.ui.language);
}

void UiViewModel::_RebuildModesVariant() {
    QVariantList modeList;

    const QString durationUnit =
        m_i18n.value(QStringLiteral("unitSec")).toString();
    for (const auto& mode : m_modes) {
        QVariantMap modeVariant;
        modeVariant.insert("id", QString::fromStdString(mode.id));
        modeVariant.insert("name", _LocalizedModeName(mode));
        modeVariant.insert("description", _LocalizedModeDescription(mode));
        modeVariant.insert("durationSeconds", mode.defaultDurationSeconds);
        modeVariant.insert("distanceUnits", mode.defaultDistanceUnits);
        modeVariant.insert("gridRows", mode.defaultGridRows);
        modeVariant.insert("gridColumns", mode.defaultGridColumns);
        modeVariant.insert("activeTargetCount", mode.defaultActiveTargetCount);
        modeVariant.insert("durationLabel",
                           QString::number(mode.defaultDurationSeconds, 'f', 0) +
                               QStringLiteral(" ") + durationUnit);
        modeVariant.insert("distanceLabel",
                           QString::number(mode.defaultDistanceUnits, 'f', 1));

        modeList.push_back(modeVariant);
    }

    m_modeList = modeList;
}

void UiViewModel::_EmitSettingsApplied() {
    if (!m_settingsChangedCallback) {
        return;
    }

    m_settingsChangedCallback(m_settings, m_viewportBackgroundColor);
}

QVariantMap
UiViewModel::_ToSessionVariant(const persistence::SessionRecord& record) const {
    return {
        {"id", static_cast<qlonglong>(record.id)},
        {"modeId", QString::fromStdString(record.modeId)},
        {"modeName", QString::fromStdString(record.modeId)},
        {"score", static_cast<qlonglong>(record.score)},
        {"elapsedSeconds", record.elapsedSeconds},
        {"shotsFired", static_cast<qlonglong>(record.shotsFired)},
        {"hits", static_cast<qlonglong>(record.hits)},
        {"misses", static_cast<qlonglong>(record.misses)},
        {"accuracyPercent", record.accuracyPercent},
        {"averageReactionTimeMs", record.averageReactionTimeMs},
        {"shotsPerSecond", record.shotsPerSecond},
        {"createdAt", _FormatIsoDateTime(record.createdAtIso)},
    };
}

QString UiViewModel::_FormatIsoDateTime(const std::string& iso) {
    const QString isoString = QString::fromStdString(iso);

    QDateTime dateTime = QDateTime::fromString(isoString, Qt::ISODateWithMs);
    if (!dateTime.isValid()) {
        dateTime = QDateTime::fromString(isoString, Qt::ISODate);
    }

    if (!dateTime.isValid()) {
        return isoString;
    }

    return dateTime.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}
}  // namespace xaimassist::ui
