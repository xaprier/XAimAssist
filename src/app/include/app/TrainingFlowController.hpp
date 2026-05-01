/**
 * @file TrainingFlowController.hpp
 * @brief Owns the training session state machine and all associated timers,
 *        pipeline-timing diagnostics, and EventBus subscriptions.
 *
 * Extracted from Application::Run() to give the training flow a single,
 * testable home.  Application::Run() constructs this class, calls
 * Initialize(), and delegates all session-related behaviour through it.
 *
 * Lifecycle:
 *   1. Construct with Deps.
 *   2. Call Initialize() — wires UI callbacks, input handlers, EventBus subs.
 *   3. Application runs the Qt event loop.
 *   4. Call Stop() during QCoreApplication::aboutToQuit.
 */

#ifndef TRAININGFLOWCONTROLLER_HPP
#define TRAININGFLOWCONTROLLER_HPP

#include <QTimer>
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"

namespace xaimassist::core {
class Logger;
}

namespace xaimassist::engine {
class Engine;
}

namespace xaimassist::input {
class InputManager;
}

namespace xaimassist::gameplay {
class ModeOrchestrator;
}

namespace xaimassist::world {
class World;
class WorldRenderSync;
}  // namespace xaimassist::world

namespace xaimassist::persistence {
class ProfileManager;
class SessionHistory;
}  // namespace xaimassist::persistence

namespace xaimassist::ui {
class UiViewModel;
}

class QWidget;

namespace xaimassist::app {

class RuntimeLoop;

/**
 * @class TrainingFlowController
 * @brief State machine for the Menu → AwaitingClick → Countdown → Active →
 *        Paused training lifecycle.
 */
class TrainingFlowController : public QObject {
    Q_OBJECT

  public:
    /// All external dependencies required by the controller.
    struct Deps {
        core::EventBus& eventBus;
        core::Logger& logger;
        engine::Engine& engine;
        input::InputManager& inputManager;
        gameplay::ModeOrchestrator& modeOrchestrator;
        world::WorldRenderSync& worldRenderSync;
        world::World& world;
        ui::UiViewModel& uiViewModel;
        RuntimeLoop& runtimeLoop;
        std::shared_ptr<persistence::ProfileManager> profileManager;
        std::shared_ptr<persistence::SessionHistory> sessionHistory;
        QWidget* viewportWidget;
        QWidget* uiOverlay;
        QWidget* window;
    };

    explicit TrainingFlowController(Deps deps, QObject* parent = nullptr);

    /// Wire all UI callbacks, input handlers, and EventBus subscriptions.
    void Initialize();

    /// Unsubscribe, stop timers, and stop the mode orderly. Called on quit.
    void Stop();

  private:
    enum class State {
        Menu,
        AwaitingSceneClick,
        Countdown,
        Active,
        Paused,
    };

    struct PipelineTimings {
        bool active{false};
        std::uint64_t sessionId{0};
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

    // ---- State transitions ----
    void _ReturnToMenuState();
    void _ExitTrainingFlow();
    void _ShowPausedOverlay();
    void _ContinueFromPause();
    void _StartActiveMode();

    // ---- Runtime loop helpers ----
    void _EnsureRuntimeLoopRunning();
    void _StopRuntimeLoop();

    // ---- UI helpers ----
    void _SetOverlayMousePassthrough(bool passthrough);
    void _RefreshSessionHistory(const std::string& modeId);

    // ---- Pipeline timing ----
    void _BeginPipelineTimings(std::uint64_t sessionId, const std::string& modeId);
    void _ResetPipelineTimings();
    void _CollectPipelineSample(std::uint64_t sessionId,
                                std::uint32_t fixedSteps,
                                double framePipelineMs,
                                double uiFrameTickMs,
                                double modeUpdateMs,
                                double worldSyncMs);
    void _LogPipelineReport(const core::events::SessionStoppedEvent& event);

    // ---- EventBus handlers ----
    void _OnFrameTick(const core::events::FrameTickEvent& tick);
    void _OnRealtimeStats(const core::events::RealtimeStatsEvent& event);
    void _OnPerformanceStats(const core::events::PerformanceStatsEvent& event);
    void _OnSessionStarted(const core::events::SessionStartedEvent& event);
    void _OnSessionStopped(const core::events::SessionStoppedEvent& event);
    void _OnSessionSummary(const core::events::SessionSummaryEvent& event);

    // ---- Dependencies (all non-owning) ----
    Deps m_deps;

    // ---- Mutable state ----
    State m_state{State::Menu};
    bool m_runtimeLoopRunning{false};

    std::string m_pendingModeId;
    double m_pendingModeDurationSeconds{0.0};
    double m_pendingModeDistanceUnits{0.0};
    std::unordered_map<std::string, double> m_pendingModeSettings;
    int m_countdownSecondsRemaining{0};

    PipelineTimings m_pipelineTimings;

    QTimer m_countdownTimer;

    core::EventBus::SubscriptionId m_frameUpdateSubId{0};
    core::EventBus::SubscriptionId m_uiEventsSubId{0};
};

}  // namespace xaimassist::app

#endif  // TRAININGFLOWCONTROLLER_HPP
