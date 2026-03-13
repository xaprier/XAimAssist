/**
 * @file StatTracker.hpp
 * @brief Real-time metric aggregation from EventBus game events.
 */

#ifndef STATTRACKER_HPP
#define STATTRACKER_HPP

#include <chrono>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "stats/SessionStats.hpp"

namespace xaimassist::core {
class Logger;
}

namespace xaimassist::stats {

/**
 * @class StatTracker
 * @brief Subscribes to shot/target/session events and maintains
 *        per-session accumulators for score, accuracy, reaction time, etc.
 *
 * Periodically publishes RealtimeStatsEvent and, on session end,
 * a SessionSummaryEvent.
 *
 * @warning THREAD SAFETY REQUIREMENT
 * StatTracker is NOT thread-safe. All event callbacks must be invoked
 * from the same thread (typically the Qt main event loop thread).
 * EventBus handlers are invoked synchronously on the publisher's thread,
 * so ensure all Publish() calls originate from the main thread.
 *
 * Debug builds include thread assertions to detect violations.
 *
 * @note If multi-threaded event publishing is required in the future,
 * add mutex protection around m_sessions and m_performanceAccumulator.
 */
class StatTracker {
  public:
    StatTracker(core::EventBus& eventBus, core::Logger& logger);
    ~StatTracker();

    // Prevent copy/move to avoid complexity
    StatTracker(const StatTracker&) = delete;
    StatTracker& operator=(const StatTracker&) = delete;
    StatTracker(StatTracker&&) = delete;
    StatTracker& operator=(StatTracker&&) = delete;

    /// Return a snapshot of the given session's metrics, or nullopt if unknown.
    /// @warning Must be called from the main thread.
    std::optional<SessionStatsSnapshot> SnapshotForSession(std::uint64_t SessionId) const;

  private:
    struct SessionAccumulator {
        SessionStatsSnapshot snapshot;
        double reactionSumMs{0.0};
        std::uint64_t reactionSampleCount{0};
        double elapsedSinceLastPublish{0.0};
        double scoreMultiplier{1.0};
        bool trackingMode{false};
        bool trackingMissPending{false};
        std::chrono::steady_clock::time_point trackingFirstMissTimestamp;
    };

    struct PerformanceAccumulator {
        double windowSeconds{0.0};
        std::uint64_t sampledFrames{0};
        double frameTimeMsSum{0.0};
        double frameTimeSquaredMsSum{0.0};
        double simulationSecondsSum{0.0};
        double worstFrameTimeMs{0.0};
        std::uint64_t missedTargetFrames{0};
        std::vector<double> frameTimesMs;
    };

    void _HandleCoreEvent(const core::events::CoreEvent& event);
    void _OnSessionStarted(const core::events::SessionStartedEvent& event);
    void _OnSessionStopped(const core::events::SessionStoppedEvent& event);
    void _OnFrameTick(const core::events::FrameTickEvent& event);
    void _OnShotFired(const core::events::ShotFiredEvent& event);
    void _OnShotHit(const core::events::ShotHitEvent& event);
    void _OnShotMiss(const core::events::ShotMissEvent& event);
    void _OnTargetSpawned(const core::events::TargetSpawnedEvent& event);
    void _OnTargetDestroyed(const core::events::TargetDestroyedEvent& event);

    void _RecordPerformanceFrame(const core::events::FrameTickEvent& event);
    void _PublishPerformance(const PerformanceAccumulator& accumulator);
    void _ResetPerformanceAccumulator();

    static void _RefreshDerivedMetrics(SessionAccumulator& accumulator);
    static double _ScoreMultiplierForSession(const core::events::SessionStartedEvent& event);
    static std::int64_t _HitScoreDelta(const SessionAccumulator& accumulator, double reactionTimeMs);
    static std::int64_t _MissScoreDelta(const SessionAccumulator& accumulator);

    void _PublishRealtime(SessionAccumulator& accumulator);
    void _PublishSummary(const SessionAccumulator& accumulator);

    SessionAccumulator* _GetSession(std::uint64_t SessionId);
    const SessionAccumulator* _GetSession(std::uint64_t SessionId) const;

    /// Assert that we're being called from the correct thread.
    void _AssertMainThread(const char* operationName) const;

    core::EventBus& m_eventBus;
    core::Logger& m_logger;
    core::EventBus::SubscriptionId m_subscriptionId{0};

    // NOT thread-safe - requires single-threaded access
    std::unordered_map<std::uint64_t, SessionAccumulator> m_sessions;
    PerformanceAccumulator m_performanceAccumulator;

#ifndef NDEBUG
    // Store the thread ID at construction for debug assertions
    std::thread::id m_constructionThreadId;
#endif
};
}  // namespace xaimassist::stats

#endif  // STATTRACKER_HPP
