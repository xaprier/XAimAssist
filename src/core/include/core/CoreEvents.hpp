/**
 * @file CoreEvents.hpp
 * @brief Defines all event types transported through the EventBus.
 *
 * Every cross-module notification (shots, targets, sessions, frames,
 * performance, lifecycle) is represented as a lightweight POD struct
 * packed into the CoreEvent variant.
 */

#ifndef COREEVENTS_HPP
#define COREEVENTS_HPP

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

namespace xaimassist::core::events {

/// Emitted when the player fires a shot.
struct ShotFiredEvent {
    std::uint64_t sessionId{0};
    std::chrono::steady_clock::time_point timestamp;
};

/// Emitted when a fired shot successfully hits a target.
struct ShotHitEvent {
    std::uint64_t sessionId{0};
    std::uint64_t targetId{0};
    double reactionTimeMs{0.0};  ///< Time between target spawn and hit.
    std::chrono::steady_clock::time_point timestamp;
};

/// Emitted when a fired shot misses all targets.
struct ShotMissEvent {
    std::uint64_t sessionId{0};
    std::chrono::steady_clock::time_point timestamp;
};

/// Emitted when a new target appears in the scene.
struct TargetSpawnedEvent {
    std::uint64_t sessionId{0};
    std::uint64_t targetId{0};
    std::chrono::steady_clock::time_point timestamp;
};

/// Emitted when a target is removed from the scene.
struct TargetDestroyedEvent {
    std::uint64_t sessionId{0};
    std::uint64_t targetId{0};
    bool destroyedByHit{false};
    std::chrono::steady_clock::time_point timestamp;
};

/// Emitted once at the beginning of a training session.
struct SessionStartedEvent {
    std::uint64_t sessionId{0};
    std::string modeId;
    std::chrono::system_clock::time_point startedAt;
    double configuredDistanceUnits{25.0};
    double targetRadius{0.5};
    std::unordered_map<std::string, double> modeSettings;
};

/// Reason a training session was terminated.
enum class SessionStopReason { Completed,
                               AbortedByUser };

/// Emitted when a training session ends (completed or aborted).
struct SessionStoppedEvent {
    std::uint64_t sessionId{0};
    std::string modeId;
    std::chrono::system_clock::time_point stoppedAt;
    double elapsedSeconds{0.0};
    double configuredDurationSeconds{0.0};
    SessionStopReason stopReason{SessionStopReason::Completed};
};

/// Periodically published real-time statistics snapshot during a session.
struct RealtimeStatsEvent {
    std::uint64_t sessionId{0};
    std::string modeId;
    double elapsedSeconds{0.0};

    std::int64_t score{0};
    std::uint64_t shotsFired{0};
    std::uint64_t hits{0};
    std::uint64_t misses{0};
    std::uint64_t targetsSpawned{0};
    std::uint64_t targetsDestroyed{0};

    double accuracyPercent{0.0};
    double averageReactionTimeMs{0.0};
    double shotsPerSecond{0.0};
};

/// Final aggregated metrics emitted after a session finishes.
struct SessionSummaryEvent {
    std::uint64_t sessionId{0};
    std::string modeId;

    double elapsedSeconds{0.0};
    std::int64_t score{0};
    std::uint64_t shotsFired{0};
    std::uint64_t hits{0};
    std::uint64_t misses{0};
    std::uint64_t targetsSpawned{0};
    std::uint64_t targetsDestroyed{0};

    double accuracyPercent{0.0};
    double averageReactionTimeMs{0.0};
    double shotsPerSecond{0.0};
};

/// Published every frame by the runtime loop with timing data.
struct FrameTickEvent {
    std::uint64_t frameIndex{0};
    double frameSeconds{0.0};     ///< Variable-step delta for this frame.
    std::uint32_t fixedSteps{0};  ///< Number of fixed-step sub-ticks.
    double fixedStepSeconds{0.0};
};

/// Periodically published rendering/frame performance metrics.
struct PerformanceStatsEvent {
    double windowSeconds{0.0};
    std::uint64_t sampledFrames{0};
    double fps{0.0};
    double averageFrameTimeMs{0.0};
    double p95FrameTimeMs{0.0};
    double worstFrameTimeMs{0.0};
    double frameJitterMs{0.0};
    double simulationLoadPercent{0.0};
    double missedTargetFramePercent{0.0};
};

/// Application-level lifecycle transitions.
enum class ApplicationLifecycleState { Starting,
                                       Running,
                                       Stopping };

/// Emitted when the application transitions between lifecycle states.
struct ApplicationLifecycleEvent {
    ApplicationLifecycleState State{ApplicationLifecycleState::Starting};
};

/// Variant holding every event type routed through the EventBus.
using CoreEvent =
    std::variant<ShotFiredEvent, ShotHitEvent, ShotMissEvent,
                 TargetSpawnedEvent, TargetDestroyedEvent, SessionStartedEvent,
                 SessionStoppedEvent, RealtimeStatsEvent, SessionSummaryEvent,
                 FrameTickEvent, PerformanceStatsEvent,
                 ApplicationLifecycleEvent>;
}  // namespace xaimassist::core::events

#endif  // COREEVENTS_HPP
