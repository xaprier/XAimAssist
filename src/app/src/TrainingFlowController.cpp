/// @file TrainingFlowController.cpp
#include "app/TrainingFlowController.hpp"

#include <QWidget>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <optional>
#include <sstream>

#include "app/PerformanceClassifier.hpp"
#include "app/RuntimeLoop.hpp"
#include "app/SettingsConverter.hpp"
#include "app/TimeUtils.hpp"
#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "core/Logger.hpp"
#include "engine/Engine.hpp"
#include "gameplay/ModeOrchestrator.hpp"
#include "input/InputManager.hpp"
#include "persistence/ProfileManager.hpp"
#include "persistence/SessionHistory.hpp"
#include "ui/UiViewModel.hpp"
#include "world/World.hpp"
#include "world/WorldRenderSync.hpp"

namespace {
constexpr double kTargetFrameTime144Ms = 1000.0 / 144.0;
constexpr double kTargetFrameTime120Ms = 1000.0 / 120.0;
constexpr int kCountdownStartSeconds = 3;
}  // namespace

namespace xaimassist::app {

TrainingFlowController::TrainingFlowController(Deps deps, QObject* parent)
    : QObject(parent), m_deps(std::move(deps)) {
    m_countdownTimer.setInterval(1000);
    m_countdownTimer.setParent(m_deps.window);
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void TrainingFlowController::Initialize() {
    // ---- Countdown timer ----
    QObject::connect(&m_countdownTimer, &QTimer::timeout, m_deps.window,
                     [this]() {
                         if (m_state != State::Countdown) {
                             m_countdownTimer.stop();
                             return;
                         }

                         --m_countdownSecondsRemaining;
                         if (m_countdownSecondsRemaining > 0) {
                             m_deps.uiViewModel.ShowTrainingCountdown(
                                 m_countdownSecondsRemaining);
                             return;
                         }

                         m_countdownTimer.stop();
                         _StartActiveMode();
                     });

    // ---- UiViewModel callbacks ----
    m_deps.uiViewModel.SetStartModeCallback(
        [this](const std::string& modeId, double durationSeconds,
               double distanceUnits,
               const std::unordered_map<std::string, double>& modeSettings) {
            if (modeId.empty()) {
                return;
            }

            if (m_deps.modeOrchestrator.HasActiveMode()) {
                m_deps.modeOrchestrator.StopMode();
            }

            m_pendingModeId = modeId;
            m_pendingModeDurationSeconds = std::max(10.0, durationSeconds);
            m_pendingModeDistanceUnits = std::clamp(distanceUnits, 1.0, 150.0);
            m_pendingModeSettings = modeSettings;
            m_countdownSecondsRemaining = 0;
            m_state = State::AwaitingSceneClick;

            m_deps.engine.SetCameraPose(engine::Engine::CameraPose{});

            m_deps.logger.Info(
                "app",
                "Training queued: mode=" + m_pendingModeId +
                    ", durationSeconds=" +
                    std::to_string(m_pendingModeDurationSeconds) +
                    ", distanceUnits=" +
                    std::to_string(m_pendingModeDistanceUnits) +
                    ", modeSettings=" +
                    std::to_string(static_cast<unsigned long long>(
                        m_pendingModeSettings.size())));

            m_deps.viewportWidget->show();
            m_deps.uiViewModel.ShowTrainingSceneAwaitingTrigger();
            _SetOverlayMousePassthrough(true);
            m_deps.inputManager.SetCaptureEnabled(false);

            _EnsureRuntimeLoopRunning();
            m_deps.worldRenderSync.Sync();
        });

    m_deps.uiViewModel.SetStopModeCallback([this]() { _ExitTrainingFlow(); });

    m_deps.uiViewModel.SetContinueTrainingCallback(
        [this]() { _ContinueFromPause(); });

    m_deps.uiViewModel.SetExitTrainingCallback(
        [this]() { _ExitTrainingFlow(); });

    m_deps.uiViewModel.SetModeSelectionChangedCallback(
        [this](const std::string& modeId) { _RefreshSessionHistory(modeId); });

    _RefreshSessionHistory(m_deps.uiViewModel.GetSelectedModeIdStd());

    // ---- Input handlers ----
    m_deps.inputManager.SetFireHandler(
        [this](const input::FireAction& action) {
            if (!action.pressed || m_state != State::Active ||
                !m_deps.modeOrchestrator.HasActiveMode()) {
                return;
            }

            if (m_deps.modeOrchestrator.ActiveModeId() == "tracking_targets") {
                return;
            }

            m_deps.eventBus.Publish(core::events::ShotFiredEvent{
                m_deps.modeOrchestrator.ActiveSessionId(),
                std::chrono::steady_clock::now()});
        });

    m_deps.inputManager.SetSceneClickHandler([this]() -> bool {
        if (m_state != State::AwaitingSceneClick) {
            return false;
        }

        if (m_pendingModeId.empty()) {
            return false;
        }

        m_state = State::Countdown;
        m_countdownSecondsRemaining = kCountdownStartSeconds;
        m_deps.uiViewModel.ShowTrainingCountdown(m_countdownSecondsRemaining);
        _SetOverlayMousePassthrough(true);
        m_countdownTimer.start();
        return true;
    });

    m_deps.inputManager.SetEscapeHandler([this]() {
        switch (m_state) {
            case State::Active:
                _ShowPausedOverlay();
                break;
            case State::AwaitingSceneClick:
            case State::Countdown:
                _ExitTrainingFlow();
                break;
            case State::Paused:
            case State::Menu:
            default:
                break;
        }
    });

    // ---- EventBus subscriptions ----
    m_frameUpdateSubId = m_deps.eventBus.Subscribe(
        [this](const core::events::CoreEvent& event) {
            const auto* tick =
                std::get_if<core::events::FrameTickEvent>(&event);
            if (tick == nullptr) {
                return;
            }

            _OnFrameTick(*tick);
        });

    m_uiEventsSubId = m_deps.eventBus.Subscribe(
        [this](const core::events::CoreEvent& event) {
            if (const auto* e =
                    std::get_if<core::events::RealtimeStatsEvent>(&event)) {
                _OnRealtimeStats(*e);
            } else if (const auto* e =
                           std::get_if<core::events::PerformanceStatsEvent>(
                               &event)) {
                _OnPerformanceStats(*e);
            } else if (const auto* e =
                           std::get_if<core::events::SessionStartedEvent>(
                               &event)) {
                _OnSessionStarted(*e);
            } else if (const auto* e =
                           std::get_if<core::events::SessionStoppedEvent>(
                               &event)) {
                _OnSessionStopped(*e);
            } else if (const auto* e =
                           std::get_if<core::events::SessionSummaryEvent>(
                               &event)) {
                _OnSessionSummary(*e);
            }
        });

    _EnsureRuntimeLoopRunning();
}

void TrainingFlowController::Stop() {
    m_countdownTimer.stop();
    _StopRuntimeLoop();

    if (m_deps.modeOrchestrator.HasActiveMode()) {
        m_deps.modeOrchestrator.StopMode();
    }

    m_deps.inputManager.SetCaptureEnabled(false);

    if (m_frameUpdateSubId != 0) {
        m_deps.eventBus.Unsubscribe(m_frameUpdateSubId);
        m_frameUpdateSubId = 0;
    }

    if (m_uiEventsSubId != 0) {
        m_deps.eventBus.Unsubscribe(m_uiEventsSubId);
        m_uiEventsSubId = 0;
    }
}

// ---------------------------------------------------------------------------
// State transitions
// ---------------------------------------------------------------------------

void TrainingFlowController::_ReturnToMenuState() {
    m_countdownTimer.stop();
    m_countdownSecondsRemaining = 0;
    m_pendingModeId.clear();
    m_pendingModeDurationSeconds = 0.0;
    m_pendingModeDistanceUnits = 0.0;
    m_pendingModeSettings.clear();

    m_state = State::Menu;
    m_deps.uiViewModel.ReturnToMainMenu();
    _SetOverlayMousePassthrough(false);
    m_deps.inputManager.SetCaptureEnabled(false);
    m_deps.viewportWidget->hide();

    _EnsureRuntimeLoopRunning();
    _RefreshSessionHistory(m_deps.uiViewModel.GetSelectedModeIdStd());
}

void TrainingFlowController::_ExitTrainingFlow() {
    if (m_deps.modeOrchestrator.HasActiveMode()) {
        m_deps.modeOrchestrator.StopMode(
            core::events::SessionStopReason::AbortedByUser);
    }

    _ReturnToMenuState();
}

void TrainingFlowController::_ShowPausedOverlay() {
    _StopRuntimeLoop();
    m_deps.inputManager.SetCaptureEnabled(false);

    m_state = State::Paused;
    m_deps.uiViewModel.ShowTrainingPaused();
    _SetOverlayMousePassthrough(false);
    m_deps.logger.Info("app", "Training paused");
}

void TrainingFlowController::_ContinueFromPause() {
    if (m_state != State::Paused) {
        return;
    }

    m_state = State::Active;
    m_deps.uiViewModel.ShowTrainingActive();
    _SetOverlayMousePassthrough(true);

    _EnsureRuntimeLoopRunning();
    m_deps.inputManager.SetCaptureEnabled(true);
    m_deps.logger.Info("app", "Training resumed");
}

void TrainingFlowController::_StartActiveMode() {
    if (m_pendingModeId.empty()) {
        m_deps.logger.Warning("app", "No pending mode id for training start");
        _ReturnToMenuState();
        return;
    }

    if (!m_deps.modeOrchestrator.StartMode(m_pendingModeId,
                                            m_pendingModeDurationSeconds,
                                            m_pendingModeDistanceUnits,
                                            m_pendingModeSettings)) {
        m_deps.logger.Error("app", "Failed to start selected game mode");
        _ReturnToMenuState();
        return;
    }

    m_pendingModeId.clear();
    m_pendingModeDurationSeconds = 0.0;
    m_pendingModeDistanceUnits = 0.0;
    m_pendingModeSettings.clear();
    m_state = State::Active;
    m_deps.uiViewModel.ShowTrainingActive();
    _SetOverlayMousePassthrough(true);
    m_deps.inputManager.SetCaptureEnabled(true);
}

// ---------------------------------------------------------------------------
// Runtime loop helpers
// ---------------------------------------------------------------------------

void TrainingFlowController::_EnsureRuntimeLoopRunning() {
    if (m_runtimeLoopRunning) {
        return;
    }

    m_deps.runtimeLoop.Start();
    m_runtimeLoopRunning = true;
}

void TrainingFlowController::_StopRuntimeLoop() {
    if (!m_runtimeLoopRunning) {
        return;
    }

    m_deps.runtimeLoop.Stop();
    m_runtimeLoopRunning = false;
}

// ---------------------------------------------------------------------------
// UI helpers
// ---------------------------------------------------------------------------

void TrainingFlowController::_SetOverlayMousePassthrough(bool passthrough) {
    m_deps.uiOverlay->setAttribute(Qt::WA_TransparentForMouseEvents,
                                    passthrough);

    if (!passthrough) {
        m_deps.uiOverlay->raise();
        m_deps.uiOverlay->setFocus(Qt::OtherFocusReason);
    }
}

void TrainingFlowController::_RefreshSessionHistory(
    const std::string& modeId) {
    if (!m_deps.sessionHistory || !m_deps.profileManager) {
        return;
    }

    const std::int64_t profileId = m_deps.profileManager->ActiveProfileId();
    if (profileId <= 0) {
        m_deps.uiViewModel.SetSessionHistory({}, std::nullopt);
        return;
    }

    const auto recentSessions =
        m_deps.sessionHistory->GetRecentSessions(profileId, 30);

    std::optional<persistence::SessionRecord> bestSession;
    if (!modeId.empty()) {
        bestSession = m_deps.sessionHistory->GetBestSessionForMode(profileId,
                                                                    modeId);
    }

    m_deps.uiViewModel.SetSessionHistory(recentSessions, bestSession);
}

// ---------------------------------------------------------------------------
// Pipeline timing
// ---------------------------------------------------------------------------

void TrainingFlowController::_BeginPipelineTimings(
    std::uint64_t sessionId, const std::string& modeId) {
    _ResetPipelineTimings();
    m_pipelineTimings.active = true;
    m_pipelineTimings.sessionId = sessionId;
    m_pipelineTimings.modeId = modeId;
}

void TrainingFlowController::_ResetPipelineTimings() {
    m_pipelineTimings = PipelineTimings{};
}

void TrainingFlowController::_CollectPipelineSample(std::uint64_t sessionId,
                                                     std::uint32_t fixedSteps,
                                                     double framePipelineMs,
                                                     double uiFrameTickMs,
                                                     double modeUpdateMs,
                                                     double worldSyncMs) {
    if (!m_pipelineTimings.active ||
        m_pipelineTimings.sessionId != sessionId) {
        return;
    }

    ++m_pipelineTimings.sampledFrames;
    m_pipelineTimings.sampledFixedSteps += fixedSteps;

    m_pipelineTimings.framePipelineMsSum += framePipelineMs;
    m_pipelineTimings.uiFrameTickMsSum += uiFrameTickMs;
    m_pipelineTimings.modeUpdateMsSum += modeUpdateMs;
    m_pipelineTimings.worldSyncMsSum += worldSyncMs;

    m_pipelineTimings.framePipelineWorstMs =
        std::max(m_pipelineTimings.framePipelineWorstMs, framePipelineMs);
    m_pipelineTimings.uiFrameTickWorstMs =
        std::max(m_pipelineTimings.uiFrameTickWorstMs, uiFrameTickMs);
    m_pipelineTimings.modeUpdateWorstMs =
        std::max(m_pipelineTimings.modeUpdateWorstMs, modeUpdateMs);
    m_pipelineTimings.worldSyncWorstMs =
        std::max(m_pipelineTimings.worldSyncWorstMs, worldSyncMs);
}

void TrainingFlowController::_LogPipelineReport(
    const core::events::SessionStoppedEvent& event) {
    if (!m_pipelineTimings.active ||
        m_pipelineTimings.sessionId != event.sessionId) {
        return;
    }

    if (m_pipelineTimings.sampledFrames == 0) {
        m_deps.logger.LogIf(
            "DEBUG_PERF", core::LogLevel::Info, "perf",
            "Session pipeline report unavailable: no collected frames");
        _ResetPipelineTimings();
        return;
    }

    const double sampledFrames =
        static_cast<double>(m_pipelineTimings.sampledFrames);
    const double avgFramePipelineMs =
        m_pipelineTimings.framePipelineMsSum / sampledFrames;
    const double avgUiFrameTickMs =
        m_pipelineTimings.uiFrameTickMsSum / sampledFrames;
    const double avgModeUpdateMs =
        m_pipelineTimings.modeUpdateMsSum / sampledFrames;
    const double avgWorldSyncMs =
        m_pipelineTimings.worldSyncMsSum / sampledFrames;
    const double avgFixedStepsPerFrame =
        static_cast<double>(m_pipelineTimings.sampledFixedSteps) / sampledFrames;
    const double estimatedFps =
        event.elapsedSeconds > 0.0
            ? sampledFrames / event.elapsedSeconds
            : 0.0;
    const double avgBudget144Pct =
        kTargetFrameTime144Ms > 0.0
            ? (avgFramePipelineMs * 100.0 / kTargetFrameTime144Ms)
            : 0.0;
    const double avgBudget120Pct =
        kTargetFrameTime120Ms > 0.0
            ? (avgFramePipelineMs * 100.0 / kTargetFrameTime120Ms)
            : 0.0;
    const double worstBudget144Pct =
        kTargetFrameTime144Ms > 0.0
            ? (m_pipelineTimings.framePipelineWorstMs * 100.0 /
               kTargetFrameTime144Ms)
            : 0.0;

    std::ostringstream report;
    report.setf(std::ios::fixed);
    report << std::setprecision(2)
           << "Session pipeline report: mode=" << event.modeId
           << ", sessionId=" << event.sessionId
           << ", elapsedSeconds=" << event.elapsedSeconds
           << ", frames=" << m_pipelineTimings.sampledFrames
           << ", estimatedFps=" << estimatedFps
           << ", avgFixedStepsPerFrame=" << avgFixedStepsPerFrame
           << ", avgFrameBudget144Pct=" << avgBudget144Pct
           << ", avgFrameBudget120Pct=" << avgBudget120Pct
           << ", worstFrameBudget144Pct=" << worstBudget144Pct
           << std::setprecision(3)
           << ", avgFramePipelineMs=" << avgFramePipelineMs
           << ", avgUiFrameTickMs=" << avgUiFrameTickMs
           << ", avgModeUpdateMs=" << avgModeUpdateMs
           << ", avgWorldSyncMs=" << avgWorldSyncMs
           << ", avgUiFrameTickUs=" << (avgUiFrameTickMs * 1000.0)
           << ", avgModeUpdateUs=" << (avgModeUpdateMs * 1000.0)
           << ", avgWorldSyncUs=" << (avgWorldSyncMs * 1000.0)
           << ", worstFramePipelineMs=" << m_pipelineTimings.framePipelineWorstMs
           << ", worstUiFrameTickMs=" << m_pipelineTimings.uiFrameTickWorstMs
           << ", worstModeUpdateMs=" << m_pipelineTimings.modeUpdateWorstMs
           << ", worstWorldSyncMs=" << m_pipelineTimings.worldSyncWorstMs;

    m_deps.logger.LogIf("DEBUG_PERF", core::LogLevel::Info, "perf",
                         report.str());
    _ResetPipelineTimings();
}

// ---------------------------------------------------------------------------
// EventBus handlers
// ---------------------------------------------------------------------------

void TrainingFlowController::_OnFrameTick(
    const core::events::FrameTickEvent& tick) {
    const auto framePipelineStart = std::chrono::steady_clock::now();

    const auto uiStart = std::chrono::steady_clock::now();
    m_deps.uiViewModel.UpdateFrameTick(tick);
    const double uiFrameTickMs =
        TimeUtils::ElapsedMilliseconds(uiStart, std::chrono::steady_clock::now());

    double modeUpdateMs = 0.0;
    if (m_state == State::Active) {
        const auto modeStart = std::chrono::steady_clock::now();
        for (std::uint32_t i = 0; i < tick.fixedSteps; ++i) {
            m_deps.modeOrchestrator.Update(tick.fixedStepSeconds);
        }
        modeUpdateMs = TimeUtils::ElapsedMilliseconds(
            modeStart, std::chrono::steady_clock::now());
    }

    double worldSyncMs = 0.0;
    if (tick.fixedSteps > 0 || m_deps.world.RenderDirtyCount() > 0) {
        const auto syncStart = std::chrono::steady_clock::now();
        m_deps.worldRenderSync.Sync();
        worldSyncMs = TimeUtils::ElapsedMilliseconds(
            syncStart, std::chrono::steady_clock::now());
    }

    if (m_deps.modeOrchestrator.HasActiveMode()) {
        _CollectPipelineSample(
            m_deps.modeOrchestrator.ActiveSessionId(),
            tick.fixedSteps,
            TimeUtils::ElapsedMilliseconds(framePipelineStart,
                                           std::chrono::steady_clock::now()),
            uiFrameTickMs, modeUpdateMs, worldSyncMs);
    }
}

void TrainingFlowController::_OnRealtimeStats(
    const core::events::RealtimeStatsEvent& event) {
    m_deps.uiViewModel.UpdateRealtimeStats(event);
}

void TrainingFlowController::_OnPerformanceStats(
    const core::events::PerformanceStatsEvent& event) {
    m_deps.uiViewModel.UpdatePerformanceStats(event);
}

void TrainingFlowController::_OnSessionStarted(
    const core::events::SessionStartedEvent& event) {
    _BeginPipelineTimings(event.sessionId, event.modeId);
    m_deps.uiViewModel.OnSessionStarted(event);
}

void TrainingFlowController::_OnSessionStopped(
    const core::events::SessionStoppedEvent& event) {
    _LogPipelineReport(event);
    m_deps.uiViewModel.OnSessionStopped(event);

    if (m_state != State::Menu) {
        m_state = State::Menu;
        m_deps.uiViewModel.ReturnToMainMenu();
        _SetOverlayMousePassthrough(false);
        m_deps.inputManager.SetCaptureEnabled(false);
        m_deps.viewportWidget->hide();
        _EnsureRuntimeLoopRunning();
    }
}

void TrainingFlowController::_OnSessionSummary(
    const core::events::SessionSummaryEvent& event) {
    const core::events::SessionSummaryEvent summary = event;

    QTimer::singleShot(0, m_deps.window, [this, summary]() {
        _RefreshSessionHistory(summary.modeId);

        const auto validation =
            persistence::SessionHistory::ValidateSessionSummary(summary);
        if (!validation.valid) {
            m_deps.uiViewModel.SetLatestResultInvalid(summary.sessionId,
                                                       validation.reasonCode);
            return;
        }

        if (!m_deps.profileManager || !m_deps.sessionHistory) {
            return;
        }

        const std::int64_t profileId =
            m_deps.profileManager->ActiveProfileId();
        if (profileId <= 0 || summary.modeId.empty()) {
            return;
        }

        const auto modePerformance =
            m_deps.sessionHistory->GetModePerformanceSummary(profileId,
                                                               summary.modeId);
        if (!modePerformance.has_value()) {
            return;
        }

        std::vector<ui::UiViewModel::PerformanceMetricComparison>
            metricComparisons;
        std::vector<PerformanceClassifier::EvaluatedMetricComparison>
            evaluatedComparisons;

        const auto appendMetric =
            [&](const std::string& metricId, double value,
                const persistence::ModeMetricAggregate& aggregate,
                PerformanceClassifier::MetricDirection direction,
                double weight) {
                const double best =
                    PerformanceClassifier::BestValueForDirection(aggregate,
                                                                  direction);
                const double worst =
                    PerformanceClassifier::WorstValueForDirection(aggregate,
                                                                   direction);
                const std::string tier =
                    PerformanceClassifier::ClassifyMetricTier(
                        modePerformance->sessionCount, value,
                        aggregate.average, best, worst, direction);

                metricComparisons.push_back(
                    ui::UiViewModel::PerformanceMetricComparison{
                        metricId, tier, value, aggregate.average, best, worst});
                evaluatedComparisons.push_back(
                    PerformanceClassifier::EvaluatedMetricComparison{tier,
                                                                      weight});
            };

        appendMetric("score", static_cast<double>(summary.score),
                     modePerformance->score,
                     PerformanceClassifier::MetricDirection::HigherIsBetter, 0.35);
        appendMetric("hits", static_cast<double>(summary.hits),
                     modePerformance->hits,
                     PerformanceClassifier::MetricDirection::HigherIsBetter, 0.10);
        appendMetric("misses", static_cast<double>(summary.misses),
                     modePerformance->misses,
                     PerformanceClassifier::MetricDirection::LowerIsBetter, 0.10);
        appendMetric("accuracyPercent", summary.accuracyPercent,
                     modePerformance->accuracyPercent,
                     PerformanceClassifier::MetricDirection::HigherIsBetter, 0.25);
        appendMetric("averageReactionTimeMs", summary.averageReactionTimeMs,
                     modePerformance->averageReactionTimeMs,
                     PerformanceClassifier::MetricDirection::LowerIsBetter, 0.15);
        appendMetric("shotsPerSecond", summary.shotsPerSecond,
                     modePerformance->shotsPerSecond,
                     PerformanceClassifier::MetricDirection::HigherIsBetter, 0.05);

        const std::string overallTier =
            PerformanceClassifier::ClassifyOverallPerformanceTier(
                modePerformance->sessionCount, evaluatedComparisons);

        m_deps.uiViewModel.SetLatestResultPerformanceBenchmark(
            summary.sessionId, overallTier, modePerformance->sessionCount,
            metricComparisons);
    });
}

}  // namespace xaimassist::app
