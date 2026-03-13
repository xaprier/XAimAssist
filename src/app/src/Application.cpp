/// @file Application.cpp
#include "app/Application.hpp"

#include <QApplication>
#include <QMainWindow>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWidget>
#include <QResource>
#include <QStackedLayout>
#include <QSurfaceFormat>
#include <QTimer>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "app/AppGameplaySettingsProvider.hpp"
#include "app/CompositionRoot.hpp"
#include "app/EngineRenderBridge.hpp"
#include "app/GameplayWorldBridge.hpp"
#include "app/PerformanceClassifier.hpp"
#include "app/RuntimeLoop.hpp"
#include "app/SettingsConverter.hpp"
#include "app/TimeUtils.hpp"
#include "app/WorldRaycastHitTest.hpp"
#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "core/Logger.hpp"
#include "engine/Engine.hpp"
#include "gameplay/BuiltInModes.hpp"
#include "gameplay/GameModeRegistry.hpp"
#include "gameplay/ModeOrchestrator.hpp"
#include "gameplay/TargetFactory.hpp"
#include "gameplay/TargetSystem.hpp"
#include "input/InputManager.hpp"
#include "persistence/AppSettings.hpp"
#include "persistence/PersistenceDatabase.hpp"
#include "persistence/ProfileManager.hpp"
#include "persistence/SessionHistory.hpp"
#include "persistence/SettingsManager.hpp"
#include "ui/UiViewModel.hpp"
#include "world/World.hpp"
#include "world/WorldRenderSync.hpp"

extern int qInitResources_ui();
extern int qCleanupResources_ui();

namespace {

enum class TrainingFlowState {
    Menu,
    AwaitingSceneClick,
    Countdown,
    Active,
    Paused
};

/// Target frame times for performance benchmarking.
constexpr double TARGET_FRAME_TIME_144_MS = 1000.0 / 144.0;
constexpr double TARGET_FRAME_TIME_120_MS = 1000.0 / 120.0;

}  // namespace

namespace xaimassist::app {
int Application::Run(int argc, char* argv[]) {
    // Initialize Qt resources from ui library
    qInitResources_ui();

    QSurfaceFormat::setDefaultFormat(engine::Engine::RecommendedSurfaceFormat());
    QQuickStyle::setStyle(QStringLiteral("Material"));

    QApplication qtApplication(argc, argv);

    CompositionRoot compositionRoot;
    RuntimeServices services = compositionRoot.CreateRuntimeServices();

    services.eventBus->Publish(core::events::ApplicationLifecycleEvent{
        core::events::ApplicationLifecycleState::Starting});
    services.logger->Info("app", "XAimAssist startup sequence initialized");
    services.logger->LogIf(
        "DEBUG_PERF", core::LogLevel::Info, "perf",
        "DEBUG_PERF enabled: session-end pipeline timing reports are active");

    const auto& runtimeSettings = services.settingsManager->Settings();

    {
        std::ostringstream settingsStream;
        settingsStream
            << "Loaded Settings: targetColor=("
            << runtimeSettings.gameplay.target.color.red << ','
            << runtimeSettings.gameplay.target.color.green << ','
            << runtimeSettings.gameplay.target.color.blue << ")"
            << ", TargetRadius=" << runtimeSettings.gameplay.target.radius
            << ", CmPer360=" << runtimeSettings.input.sensitivity.cmPer360
            << ", Dpi=" << runtimeSettings.input.sensitivity.dpi
            << ", SensitivityScale="
            << runtimeSettings.input.sensitivity.sensitivityScale
            << ", Theme=" << SettingsConverter::ThemeModeToString(runtimeSettings.ui.themeMode)
            << ", language=" << SettingsConverter::LanguageToString(runtimeSettings.ui.language);
        services.logger->Info("Settings", settingsStream.str());
    }

    if (services.profileManager) {
        if (const auto ActiveProfile = services.profileManager->ActiveProfile();
            ActiveProfile.has_value()) {
            std::ostringstream profileStream;
            profileStream << "Active profile: id=" << ActiveProfile->id
                          << ", name=" << ActiveProfile->name;
            services.logger->Info("persistence", profileStream.str());
        }
    }

    if (services.persistenceDatabase) {
        services.logger->Info("persistence",
                              "SQLite DB: " +
                                  services.persistenceDatabase->DatabasePath());
    }

    ui::UiViewModel uiViewModel(runtimeSettings);
    uiViewModel.ReturnToMainMenu();

    QMainWindow window;
    window.setWindowTitle(QStringLiteral("XAimAssist - Phase 11"));
    window.resize(1480, 860);

    auto* viewportWidget = services.engine->CreateViewport(&window);
    if (viewportWidget == nullptr) {
        services.logger->Error("app", "Engine viewport initialization failed");
        return EXIT_FAILURE;
    }

    auto* uiOverlay = new QQuickWidget(&window);
    uiOverlay->setResizeMode(QQuickWidget::SizeRootObjectToView);
    uiOverlay->setClearColor(Qt::transparent);
    uiOverlay->setAttribute(Qt::WA_TranslucentBackground, true);
    uiOverlay->setAttribute(Qt::WA_AlwaysStackOnTop, true);
    uiOverlay->setFocusPolicy(Qt::StrongFocus);
    uiOverlay->rootContext()->setContextProperty(QStringLiteral("viewModel"),
                                                 &uiViewModel);
    uiOverlay->setSource(QUrl(QStringLiteral("qrc:/xaimassist/ui/MainView.qml")));

    if (uiOverlay->status() == QQuickWidget::Error) {
        for (const auto& Error : uiOverlay->errors()) {
            services.logger->Error("ui", Error.toString().toStdString());
        }
    }

    auto* layeredContainer = new QWidget(&window);
    auto* layeredLayout = new QStackedLayout(layeredContainer);
    layeredLayout->setStackingMode(QStackedLayout::StackAll);
    layeredLayout->setContentsMargins(0, 0, 0, 0);
    layeredLayout->addWidget(viewportWidget);
    layeredLayout->addWidget(uiOverlay);
    window.setCentralWidget(layeredContainer);

    viewportWidget->hide();

    world::World world;
    EngineRenderBridge renderBridge(*services.engine);
    world::WorldRenderSync worldRenderSync(world, renderBridge);

    GameplayWorldBridge gameplayWorldBridge(world);
    WorldRaycastHitTest worldRaycastHitTest(world, *services.engine);
    AppGameplaySettingsProvider gameplaySettingsProvider(
        *services.settingsManager);

    gameplay::TargetFactory targetFactory;
    gameplay::TargetSystem TargetSystem(*services.eventBus, *services.logger,
                                        gameplayWorldBridge, worldRaycastHitTest,
                                        targetFactory);

    gameplay::GameModeRegistry gameModeRegistry;
    gameplay::RegisterBuiltInModes(gameModeRegistry);
    gameplay::ModeOrchestrator modeOrchestrator(
        *services.eventBus, *services.logger, gameModeRegistry,
        gameplayWorldBridge, TargetSystem, gameplaySettingsProvider);

    uiViewModel.SetModes(SettingsConverter::ToModeDescriptors(gameModeRegistry.AvailableModes()));

    RuntimeLoop runtimeLoop(*services.frameClock, *services.eventBus);
    bool runtimeLoopRunning = false;

    auto ensureRuntimeLoopRunning = [&]() {
        if (runtimeLoopRunning) {
            return;
        }

        runtimeLoop.Start();
        runtimeLoopRunning = true;
    };

    auto stopRuntimeLoop = [&]() {
        if (!runtimeLoopRunning) {
            return;
        }

        runtimeLoop.Stop();
        runtimeLoopRunning = false;
    };

    auto refreshSessionHistory = [&](const std::string& modeId) {
        if (!services.sessionHistory || !services.profileManager) {
            return;
        }

        const std::int64_t ActiveProfileId =
            services.profileManager->ActiveProfileId();
        if (ActiveProfileId <= 0) {
            uiViewModel.SetSessionHistory({}, std::nullopt);
            return;
        }

        const auto RecentSessions =
            services.sessionHistory->GetRecentSessions(ActiveProfileId, 30);

        std::optional<persistence::SessionRecord> BestSession;
        if (!modeId.empty()) {
            BestSession = services.sessionHistory->GetBestSessionForMode(
                ActiveProfileId, modeId);
        }

        uiViewModel.SetSessionHistory(RecentSessions, BestSession);
    };

    auto applyRuntimeSettings =
        [&](const persistence::AppSettings& Settings,
            const std::array<double, 3>& viewportBackground,
            bool persistSettings) {
            if (services.settingsManager) {
                services.settingsManager->SetSettings(Settings);
                if (persistSettings && !services.settingsManager->Save()) {
                    services.logger->Warning("persistence",
                                             "Failed to Save Settings changes");
                }
            }

            services.inputManager->SetSensitivitySettings(
                SettingsConverter::ToSensitivitySettings(Settings));
            services.inputManager->SetRawInputEnabled(
                Settings.input.rawInputEnabled);

            TargetSystem.UpdateAllTargetColors(
                {Settings.gameplay.target.color.red,
                 Settings.gameplay.target.color.green,
                 Settings.gameplay.target.color.blue});

            services.engine->SetBackgroundColor(viewportBackground);
        };

    auto setOverlayMousePassthrough = [&](bool passthrough) {
        uiOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, passthrough);

        if (!passthrough) {
            uiOverlay->raise();
            uiOverlay->setFocus(Qt::OtherFocusReason);
        }
    };

    TrainingFlowState trainingFlowState = TrainingFlowState::Menu;
    std::string pendingModeId;
    double pendingModeDurationSeconds = 0.0;
    double pendingModeDistanceUnits = 0.0;
    std::unordered_map<std::string, double> pendingModeSettings;
    int countdownSecondsRemaining = 0;

    QTimer countdownTimer(&window);
    countdownTimer.setInterval(1000);

    struct SessionPipelineTimings {
        bool active{false};
        std::uint64_t SessionId{0};
        std::string modeId;

        std::uint64_t sampledFrames{0};
        std::uint64_t sampledFixedSteps{0};

        double framePipelineMsSum{0.0};
        double uiFrameTickMsSum{0.0};
        double modeUpdateMsSum{0.0};
        double worldSyncMsSum{0.0};

        double framePipelineWorstMs{0.0};
        double uiFrameTickWorstMs{0.0};
        double modeUpdateWorstMs{0.0};
        double worldSyncWorstMs{0.0};
    };

    SessionPipelineTimings sessionPipelineTimings;

    auto resetSessionPipelineTimings = [&]() {
        sessionPipelineTimings = SessionPipelineTimings{};
    };

    auto beginSessionPipelineTimings = [&](std::uint64_t SessionId,
                                           const std::string& modeId) {
        resetSessionPipelineTimings();
        sessionPipelineTimings.active = true;
        sessionPipelineTimings.SessionId = SessionId;
        sessionPipelineTimings.modeId = modeId;
    };

    auto collectSessionPipelineSample =
        [&](std::uint64_t SessionId, std::uint32_t fixedSteps,
            double framePipelineMs, double uiFrameTickMs, double modeUpdateMs,
            double worldSyncMs) {
            if (!sessionPipelineTimings.active ||
                sessionPipelineTimings.SessionId != SessionId) {
                return;
            }

            sessionPipelineTimings.sampledFrames += 1;
            sessionPipelineTimings.sampledFixedSteps += fixedSteps;

            sessionPipelineTimings.framePipelineMsSum += framePipelineMs;
            sessionPipelineTimings.uiFrameTickMsSum += uiFrameTickMs;
            sessionPipelineTimings.modeUpdateMsSum += modeUpdateMs;
            sessionPipelineTimings.worldSyncMsSum += worldSyncMs;

            sessionPipelineTimings.framePipelineWorstMs = std::max(
                sessionPipelineTimings.framePipelineWorstMs, framePipelineMs);
            sessionPipelineTimings.uiFrameTickWorstMs =
                std::max(sessionPipelineTimings.uiFrameTickWorstMs, uiFrameTickMs);
            sessionPipelineTimings.modeUpdateWorstMs =
                std::max(sessionPipelineTimings.modeUpdateWorstMs, modeUpdateMs);
            sessionPipelineTimings.worldSyncWorstMs =
                std::max(sessionPipelineTimings.worldSyncWorstMs, worldSyncMs);
        };

    auto logSessionPipelineReport =
        [&](const core::events::SessionStoppedEvent& event) {
            if (!sessionPipelineTimings.active ||
                sessionPipelineTimings.SessionId != event.sessionId) {
                return;
            }

            if (sessionPipelineTimings.sampledFrames == 0) {
                services.logger->LogIf(
                    "DEBUG_PERF", core::LogLevel::Info, "perf",
                    "Session pipeline report unavailable: no collected frames");
                resetSessionPipelineTimings();
                return;
            }

            const double sampledFrames =
                static_cast<double>(sessionPipelineTimings.sampledFrames);
            const double averageFramePipelineMs =
                sessionPipelineTimings.framePipelineMsSum / sampledFrames;
            const double averageUiFrameTickMs =
                sessionPipelineTimings.uiFrameTickMsSum / sampledFrames;
            const double averageModeUpdateMs =
                sessionPipelineTimings.modeUpdateMsSum / sampledFrames;
            const double averageWorldSyncMs =
                sessionPipelineTimings.worldSyncMsSum / sampledFrames;
            const double averageFixedStepsPerFrame =
                static_cast<double>(sessionPipelineTimings.sampledFixedSteps) /
                sampledFrames;
            const double estimatedFps = event.elapsedSeconds > 0.0
                                            ? sampledFrames / event.elapsedSeconds
                                            : 0.0;
            const double avgFrameBudgetUsage144Percent =
                TARGET_FRAME_TIME_144_MS > 0.0
                    ? (averageFramePipelineMs * 100.0 / TARGET_FRAME_TIME_144_MS)
                    : 0.0;
            const double avgFrameBudgetUsage120Percent =
                TARGET_FRAME_TIME_120_MS > 0.0
                    ? (averageFramePipelineMs * 100.0 / TARGET_FRAME_TIME_120_MS)
                    : 0.0;
            const double worstFrameBudgetUsage144Percent =
                TARGET_FRAME_TIME_144_MS > 0.0
                    ? (sessionPipelineTimings.framePipelineWorstMs * 100.0 /
                       TARGET_FRAME_TIME_144_MS)
                    : 0.0;

            std::ostringstream report;
            report.setf(std::ios::fixed);
            report << std::setprecision(2)
                   << "Session pipeline report: mode=" << event.modeId
                   << ", SessionId=" << event.sessionId
                   << ", ElapsedSeconds=" << event.elapsedSeconds
                   << ", frames=" << sessionPipelineTimings.sampledFrames
                   << ", estimatedFps=" << estimatedFps
                   << ", avgFixedStepsPerFrame=" << averageFixedStepsPerFrame
                   << ", avgFrameBudget144Pct=" << avgFrameBudgetUsage144Percent
                   << ", avgFrameBudget120Pct=" << avgFrameBudgetUsage120Percent
                   << ", worstFrameBudget144Pct=" << worstFrameBudgetUsage144Percent
                   << std::setprecision(3)
                   << ", avgFramePipelineMs=" << averageFramePipelineMs
                   << ", avgUiFrameTickMs=" << averageUiFrameTickMs
                   << ", avgModeUpdateMs=" << averageModeUpdateMs
                   << ", avgWorldSyncMs=" << averageWorldSyncMs
                   << ", avgUiFrameTickUs=" << (averageUiFrameTickMs * 1000.0)
                   << ", avgModeUpdateUs=" << (averageModeUpdateMs * 1000.0)
                   << ", avgWorldSyncUs=" << (averageWorldSyncMs * 1000.0)
                   << ", worstFramePipelineMs="
                   << sessionPipelineTimings.framePipelineWorstMs
                   << ", worstUiFrameTickMs="
                   << sessionPipelineTimings.uiFrameTickWorstMs
                   << ", worstModeUpdateMs="
                   << sessionPipelineTimings.modeUpdateWorstMs
                   << ", worstWorldSyncMs="
                   << sessionPipelineTimings.worldSyncWorstMs;

            services.logger->LogIf("DEBUG_PERF", core::LogLevel::Info, "perf",
                                   report.str());
            resetSessionPipelineTimings();
        };

    auto returnToMenuState = [&]() {
        countdownTimer.stop();
        countdownSecondsRemaining = 0;
        pendingModeId.clear();
        pendingModeDurationSeconds = 0.0;
        pendingModeDistanceUnits = 0.0;
        pendingModeSettings.clear();

        trainingFlowState = TrainingFlowState::Menu;
        uiViewModel.ReturnToMainMenu();
        setOverlayMousePassthrough(false);
        services.inputManager->SetCaptureEnabled(false);
        viewportWidget->hide();

        ensureRuntimeLoopRunning();
        refreshSessionHistory(uiViewModel.GetSelectedModeIdStd());
    };

    auto exitTrainingFlow = [&]() {
        if (modeOrchestrator.HasActiveMode()) {
            modeOrchestrator.StopMode(core::events::SessionStopReason::AbortedByUser);
        }

        returnToMenuState();
    };

    auto showPausedOverlay = [&]() {
        stopRuntimeLoop();
        services.inputManager->SetCaptureEnabled(false);

        trainingFlowState = TrainingFlowState::Paused;
        uiViewModel.ShowTrainingPaused();
        setOverlayMousePassthrough(false);
        services.logger->Info("app", "Training paused");
    };

    auto continueFromPause = [&]() {
        if (trainingFlowState != TrainingFlowState::Paused) {
            return;
        }

        trainingFlowState = TrainingFlowState::Active;
        uiViewModel.ShowTrainingActive();
        setOverlayMousePassthrough(true);

        ensureRuntimeLoopRunning();
        services.inputManager->SetCaptureEnabled(true);
        services.logger->Info("app", "Training resumed");
    };

    auto startActiveMode = [&]() {
        if (pendingModeId.empty()) {
            services.logger->Warning("app", "No pending mode id for training Start");
            returnToMenuState();
            return;
        }

        if (!modeOrchestrator.StartMode(pendingModeId, pendingModeDurationSeconds,
                                        pendingModeDistanceUnits,
                                        pendingModeSettings)) {
            services.logger->Error("app", "Failed to Start selected game mode");
            returnToMenuState();
            return;
        }

        pendingModeId.clear();
        pendingModeDurationSeconds = 0.0;
        pendingModeDistanceUnits = 0.0;
        pendingModeSettings.clear();
        trainingFlowState = TrainingFlowState::Active;
        uiViewModel.ShowTrainingActive();
        setOverlayMousePassthrough(true);
        services.inputManager->SetCaptureEnabled(true);
    };

    QObject::connect(&countdownTimer, &QTimer::timeout, &window, [&]() {
        if (trainingFlowState != TrainingFlowState::Countdown) {
            countdownTimer.stop();
            return;
        }

        countdownSecondsRemaining -= 1;
        if (countdownSecondsRemaining > 0) {
            uiViewModel.ShowTrainingCountdown(countdownSecondsRemaining);
            return;
        }

        countdownTimer.stop();
        startActiveMode();
    });

    applyRuntimeSettings(runtimeSettings,
                         uiViewModel.GetViewportBackgroundColor(), false);

    uiViewModel.SetSettingsChangedCallback(
        [&](const persistence::AppSettings& Settings,
            const std::array<double, 3>& viewportBackground) {
            applyRuntimeSettings(Settings, viewportBackground, true);
        });

    uiViewModel.SetStartModeCallback(
        [&](const std::string& modeId, double durationSeconds,
            double distanceUnits,
            const std::unordered_map<std::string, double>& ModeSettings) {
            if (modeId.empty()) {
                return;
            }

            if (modeOrchestrator.HasActiveMode()) {
                modeOrchestrator.StopMode();
            }

            pendingModeId = modeId;
            pendingModeDurationSeconds = std::max(10.0, durationSeconds);
            pendingModeDistanceUnits = std::clamp(distanceUnits, 1.0, 150.0);
            pendingModeSettings = ModeSettings;
            countdownSecondsRemaining = 0;
            trainingFlowState = TrainingFlowState::AwaitingSceneClick;

            services.engine->SetCameraPose(engine::Engine::CameraPose{});

            services.logger->Info(
                "app",
                "Training queued: mode=" + pendingModeId + ", durationSeconds=" +
                    std::to_string(pendingModeDurationSeconds) +
                    ", distanceUnits=" + std::to_string(pendingModeDistanceUnits) +
                    ", ModeSettings=" +
                    std::to_string(static_cast<unsigned long long>(
                        pendingModeSettings.size())));

            viewportWidget->show();
            uiViewModel.ShowTrainingSceneAwaitingTrigger();
            setOverlayMousePassthrough(true);
            services.inputManager->SetCaptureEnabled(false);

            ensureRuntimeLoopRunning();
            worldRenderSync.Sync();
        });

    uiViewModel.SetStopModeCallback([&]() { exitTrainingFlow(); });

    uiViewModel.SetContinueTrainingCallback([&]() { continueFromPause(); });

    uiViewModel.SetExitTrainingCallback([&]() { exitTrainingFlow(); });

    uiViewModel.SetModeSelectionChangedCallback(
        [&](const std::string& modeId) { refreshSessionHistory(modeId); });

    refreshSessionHistory(uiViewModel.GetSelectedModeIdStd());

    services.inputManager->SetLookHandler([&](const input::LookAction& action) {
        services.engine->ApplyCameraLookDelta(action.yawDeltaDegrees,
                                              action.pitchDeltaDegrees);
    });

    services.inputManager->SetFireHandler([&](const input::FireAction& action) {
        if (!action.pressed || trainingFlowState != TrainingFlowState::Active ||
            !modeOrchestrator.HasActiveMode()) {
            return;
        }

        if (modeOrchestrator.ActiveModeId() == "tracking_targets") {
            return;
        }

        services.eventBus->Publish(core::events::ShotFiredEvent{
            modeOrchestrator.ActiveSessionId(), std::chrono::steady_clock::now()});
    });

    services.inputManager->SetSceneClickHandler([&]() -> bool {
        if (trainingFlowState != TrainingFlowState::AwaitingSceneClick) {
            return false;
        }

        if (pendingModeId.empty()) {
            return false;
        }

        trainingFlowState = TrainingFlowState::Countdown;
        countdownSecondsRemaining = 3;
        uiViewModel.ShowTrainingCountdown(countdownSecondsRemaining);
        setOverlayMousePassthrough(true);
        countdownTimer.start();
        return true;
    });

    services.inputManager->SetEscapeHandler([&]() {
        switch (trainingFlowState) {
            case TrainingFlowState::Active:
                showPausedOverlay();
                break;
            case TrainingFlowState::Paused:
                break;
            case TrainingFlowState::AwaitingSceneClick:
            case TrainingFlowState::Countdown:
                exitTrainingFlow();
                break;
            case TrainingFlowState::Menu:
            default:
                break;
        }
    });

    services.inputManager->AttachViewport(viewportWidget);
    services.inputManager->SetCaptureEnabled(false);

    const auto frameUpdateSubscription = services.eventBus->Subscribe(
        [&modeOrchestrator, &worldRenderSync, &uiViewModel, &trainingFlowState,
         &world,
         &collectSessionPipelineSample](const core::events::CoreEvent& event) {
            const auto* frameTickEvent =
                std::get_if<core::events::FrameTickEvent>(&event);
            if (frameTickEvent == nullptr) {
                return;
            }

            const auto framePipelineStart = std::chrono::steady_clock::now();

            const auto uiFrameTickStart = std::chrono::steady_clock::now();
            uiViewModel.UpdateFrameTick(*frameTickEvent);
            const double uiFrameTickMs = TimeUtils::ElapsedMilliseconds(
                uiFrameTickStart, std::chrono::steady_clock::now());

            double modeUpdateMs = 0.0;

            if (trainingFlowState == TrainingFlowState::Active) {
                const auto modeUpdateStart = std::chrono::steady_clock::now();
                for (std::uint32_t fixedStep = 0;
                     fixedStep < frameTickEvent->fixedSteps; ++fixedStep) {
                    modeOrchestrator.Update(frameTickEvent->fixedStepSeconds);
                }
                modeUpdateMs = TimeUtils::ElapsedMilliseconds(modeUpdateStart,
                                                              std::chrono::steady_clock::now());
            }

            double worldSyncMs = 0.0;

            if (frameTickEvent->fixedSteps > 0 || world.RenderDirtyCount() > 0) {
                const auto worldSyncStart = std::chrono::steady_clock::now();
                worldRenderSync.Sync();
                worldSyncMs = TimeUtils::ElapsedMilliseconds(worldSyncStart,
                                                             std::chrono::steady_clock::now());
            }

            if (modeOrchestrator.HasActiveMode()) {
                collectSessionPipelineSample(
                    modeOrchestrator.ActiveSessionId(), frameTickEvent->fixedSteps,
                    TimeUtils::ElapsedMilliseconds(framePipelineStart,
                                                   std::chrono::steady_clock::now()),
                    uiFrameTickMs, modeUpdateMs, worldSyncMs);
            }
        });

    const auto uiEventsSubscription = services.eventBus->Subscribe(
        [&uiViewModel, &refreshSessionHistory, &trainingFlowState,
         &setOverlayMousePassthrough, &viewportWidget, &services,
         &ensureRuntimeLoopRunning, &beginSessionPipelineTimings,
         &logSessionPipelineReport,
         &window](const core::events::CoreEvent& event) {
            if (const auto* realtimeStatsEvent =
                    std::get_if<core::events::RealtimeStatsEvent>(&event)) {
                uiViewModel.UpdateRealtimeStats(*realtimeStatsEvent);
                return;
            }

            if (const auto* performanceStatsEvent =
                    std::get_if<core::events::PerformanceStatsEvent>(&event)) {
                uiViewModel.UpdatePerformanceStats(*performanceStatsEvent);
                return;
            }

            if (const auto* sessionStartedEvent =
                    std::get_if<core::events::SessionStartedEvent>(&event)) {
                beginSessionPipelineTimings(sessionStartedEvent->sessionId,
                                            sessionStartedEvent->modeId);
                uiViewModel.OnSessionStarted(*sessionStartedEvent);
                return;
            }

            if (const auto* sessionStoppedEvent =
                    std::get_if<core::events::SessionStoppedEvent>(&event)) {
                logSessionPipelineReport(*sessionStoppedEvent);
                uiViewModel.OnSessionStopped(*sessionStoppedEvent);

                if (trainingFlowState != TrainingFlowState::Menu) {
                    trainingFlowState = TrainingFlowState::Menu;
                    uiViewModel.ReturnToMainMenu();
                    setOverlayMousePassthrough(false);
                    services.inputManager->SetCaptureEnabled(false);
                    viewportWidget->hide();
                    ensureRuntimeLoopRunning();
                }

                return;
            }

            if (const auto* sessionSummaryEvent =
                    std::get_if<core::events::SessionSummaryEvent>(&event)) {
                const core::events::SessionSummaryEvent summary =
                    *sessionSummaryEvent;
                QTimer::singleShot(0, &window, [&, summary]() {
                    refreshSessionHistory(summary.modeId);

                    const auto validation =
                        persistence::SessionHistory::ValidateSessionSummary(summary);
                    if (!validation.valid) {
                        uiViewModel.SetLatestResultInvalid(summary.sessionId,
                                                           validation.reasonCode);
                        return;
                    }

                    if (!services.profileManager || !services.sessionHistory) {
                        return;
                    }

                    const std::int64_t ActiveProfileId =
                        services.profileManager->ActiveProfileId();
                    if (ActiveProfileId <= 0 || summary.modeId.empty()) {
                        return;
                    }

                    const auto modePerformance =
                        services.sessionHistory->GetModePerformanceSummary(
                            ActiveProfileId, summary.modeId);
                    if (!modePerformance.has_value()) {
                        return;
                    }

                    std::vector<ui::UiViewModel::PerformanceMetricComparison>
                        metricComparisons;
                    std::vector<PerformanceClassifier::EvaluatedMetricComparison> evaluatedComparisons;

                    const auto appendMetricComparison =
                        [&](const std::string& metricId, double metricValue,
                            const persistence::ModeMetricAggregate& aggregate,
                            PerformanceClassifier::MetricDirection direction, double weight) {
                            const double bestValue =
                                PerformanceClassifier::BestValueForDirection(aggregate, direction);
                            const double worstValue =
                                PerformanceClassifier::WorstValueForDirection(aggregate, direction);
                            const std::string metricTier = PerformanceClassifier::ClassifyMetricTier(
                                modePerformance->sessionCount, metricValue,
                                aggregate.average, bestValue, worstValue, direction);

                            metricComparisons.push_back(
                                ui::UiViewModel::PerformanceMetricComparison{
                                    metricId, metricTier, metricValue, aggregate.average,
                                    bestValue, worstValue});
                            evaluatedComparisons.push_back(
                                PerformanceClassifier::EvaluatedMetricComparison{metricTier, weight});
                        };

                    appendMetricComparison("score", static_cast<double>(summary.score),
                                           modePerformance->score,
                                           PerformanceClassifier::MetricDirection::HigherIsBetter, 0.35);
                    appendMetricComparison("hits", static_cast<double>(summary.hits),
                                           modePerformance->hits,
                                           PerformanceClassifier::MetricDirection::HigherIsBetter, 0.10);
                    appendMetricComparison(
                        "misses", static_cast<double>(summary.misses),
                        modePerformance->misses, PerformanceClassifier::MetricDirection::LowerIsBetter, 0.10);
                    appendMetricComparison("accuracyPercent", summary.accuracyPercent,
                                           modePerformance->accuracyPercent,
                                           PerformanceClassifier::MetricDirection::HigherIsBetter, 0.25);
                    appendMetricComparison("averageReactionTimeMs",
                                           summary.averageReactionTimeMs,
                                           modePerformance->averageReactionTimeMs,
                                           PerformanceClassifier::MetricDirection::LowerIsBetter, 0.15);
                    appendMetricComparison("shotsPerSecond", summary.shotsPerSecond,
                                           modePerformance->shotsPerSecond,
                                           PerformanceClassifier::MetricDirection::HigherIsBetter, 0.05);

                    const std::string overallTier = PerformanceClassifier::ClassifyOverallPerformanceTier(
                        modePerformance->sessionCount, evaluatedComparisons);

                    uiViewModel.SetLatestResultPerformanceBenchmark(
                        summary.sessionId, overallTier, modePerformance->sessionCount,
                        metricComparisons);
                });
            }
        });

    worldRenderSync.Sync();
    ensureRuntimeLoopRunning();
    setOverlayMousePassthrough(false);

    services.eventBus->Publish(core::events::ApplicationLifecycleEvent{
        core::events::ApplicationLifecycleState::Running});
    services.logger->Info("app", "Runtime loop started");

    QObject::connect(&qtApplication, &QCoreApplication::aboutToQuit, [&]() {
        countdownTimer.stop();
        stopRuntimeLoop();

        if (modeOrchestrator.HasActiveMode()) {
            modeOrchestrator.StopMode();
        }

        services.inputManager->SetCaptureEnabled(false);
        services.inputManager->DetachViewport();
        services.eventBus->Unsubscribe(frameUpdateSubscription);
        services.eventBus->Unsubscribe(uiEventsSubscription);
        worldRenderSync.Reset();
        services.engine->Shutdown();
        services.eventBus->Publish(core::events::ApplicationLifecycleEvent{
            core::events::ApplicationLifecycleState::Stopping});
        services.logger->Info("app", "XAimAssist Shutdown sequence completed");
    });

    window.show();
    return qtApplication.exec();
}
}  // namespace xaimassist::app
