/// @file StatTracker.cpp
#include "stats/StatTracker.hpp"

#include <assert.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include "core/EventBus.hpp"
#include "core/Logger.hpp"

// Qt thread checking (if available)
#ifdef QT_CORE_LIB
#include <QThread>
#endif

namespace {
constexpr double REALTIME_PUBLISH_INTERVAL_SECONDS = 0.25;
constexpr double PERFORMANCE_PUBLISH_INTERVAL_SECONDS = 0.5;
constexpr double TARGET_FRAME_TIME_MS = 1000.0 / 144.0;  // 144 Hz baseline
constexpr double BASE_HIT_POINTS = 40.0;
constexpr double BASE_MISS_POINTS = 20.0;
constexpr double TRACKING_FALLBACK_REACTION_MS = 150.0;

double ModeSettingValue(const std::unordered_map<std::string, double>& ModeSettings, const char* key, double fallback) {
    const auto iterator = ModeSettings.find(key);
    if (iterator == ModeSettings.end()) {
        return fallback;
    }

    return iterator->second;
}
}  // namespace

namespace xaimassist::stats {
StatTracker::StatTracker(core::EventBus& eventBus, core::Logger& logger) : m_eventBus(eventBus), m_logger(logger) {
#ifndef NDEBUG
    m_constructionThreadId = std::this_thread::get_id();
    m_logger.Debug("stats", "StatTracker constructed - enforcing single-threaded access");
#endif

    m_performanceAccumulator.frameTimesMs.reserve(256);
    m_subscriptionId =
        m_eventBus.Subscribe([this](const core::events::CoreEvent& event) {
            _AssertMainThread("EventBus callback");
            _HandleCoreEvent(event);
        });
}

StatTracker::~StatTracker() {
    _AssertMainThread("Destructor");

    if (m_subscriptionId != 0) {
        m_eventBus.Unsubscribe(m_subscriptionId);
        m_subscriptionId = 0;
    }

    m_logger.Debug("stats", "StatTracker destroyed");
}

std::optional<SessionStatsSnapshot> StatTracker::SnapshotForSession(std::uint64_t SessionId) const {
    _AssertMainThread("SnapshotForSession");

    const auto* accumulator = _GetSession(SessionId);
    if (accumulator == nullptr) {
        return std::nullopt;
    }

    return accumulator->snapshot;
}

void StatTracker::_AssertMainThread(const char* operationName) const {
#ifndef NDEBUG
    const auto currentThreadId = std::this_thread::get_id();

    if (currentThreadId != m_constructionThreadId) {
        m_logger.Error("stats",
                       std::string("THREAD SAFETY VIOLATION: ") + operationName +
                           " called from wrong thread!");

        // Qt-specific assertion if available
#ifdef QT_CORE_LIB
        if (QCoreApplication::instance()) {
            const bool isMainThread =
                QThread::currentThread() == QCoreApplication::instance()->thread();

            if (!isMainThread) {
                m_logger.Error("stats",
                               "Not on Qt main thread - this will cause data races!");
            }

            // Hard assertion in debug mode
            Q_ASSERT_X(isMainThread,
                       "StatTracker",
                       "StatTracker methods must be called from Qt main thread");
        }
#endif

        // Standard assertion
        assert(currentThreadId == m_constructionThreadId &&
               "StatTracker: Thread safety violation detected!");
    }
#else
    // Release build: no-op (operationName unused)
    (void)operationName;
#endif
}

void StatTracker::_HandleCoreEvent(const core::events::CoreEvent& event) {
    if (const auto* sessionStartedEvent =
            std::get_if<core::events::SessionStartedEvent>(&event)) {
        _OnSessionStarted(*sessionStartedEvent);
        return;
    }

    if (const auto* sessionStoppedEvent =
            std::get_if<core::events::SessionStoppedEvent>(&event)) {
        _OnSessionStopped(*sessionStoppedEvent);
        return;
    }

    if (const auto* frameTickEvent =
            std::get_if<core::events::FrameTickEvent>(&event)) {
        _OnFrameTick(*frameTickEvent);
        return;
    }

    if (const auto* shotFiredEvent =
            std::get_if<core::events::ShotFiredEvent>(&event)) {
        _OnShotFired(*shotFiredEvent);
        return;
    }

    if (const auto* shotHitEvent =
            std::get_if<core::events::ShotHitEvent>(&event)) {
        _OnShotHit(*shotHitEvent);
        return;
    }

    if (const auto* shotMissEvent =
            std::get_if<core::events::ShotMissEvent>(&event)) {
        _OnShotMiss(*shotMissEvent);
        return;
    }

    if (const auto* targetSpawnedEvent =
            std::get_if<core::events::TargetSpawnedEvent>(&event)) {
        _OnTargetSpawned(*targetSpawnedEvent);
        return;
    }

    if (const auto* targetDestroyedEvent =
            std::get_if<core::events::TargetDestroyedEvent>(&event)) {
        _OnTargetDestroyed(*targetDestroyedEvent);
    }
}

void StatTracker::_OnSessionStarted(const core::events::SessionStartedEvent& event) {
    SessionAccumulator accumulator;
    accumulator.snapshot.SessionId = event.sessionId;
    accumulator.snapshot.modeId = event.modeId;
    accumulator.snapshot.active = true;
    accumulator.scoreMultiplier = _ScoreMultiplierForSession(event);
    accumulator.trackingMode = event.modeId == "tracking_targets";
    _RefreshDerivedMetrics(accumulator);

    m_sessions[event.sessionId] = std::move(accumulator);
    if (auto* current = _GetSession(event.sessionId)) {
        _PublishRealtime(*current);
    }
}

void StatTracker::_OnSessionStopped(
    const core::events::SessionStoppedEvent& event) {
    auto* accumulator = _GetSession(event.sessionId);
    if (accumulator == nullptr) {
        return;
    }

    accumulator->snapshot.active = false;
    _RefreshDerivedMetrics(*accumulator);
    _PublishRealtime(*accumulator);
    _PublishSummary(*accumulator);

    m_logger.Info("stats", "Session summary emitted");
}

void StatTracker::_OnFrameTick(const core::events::FrameTickEvent& event) {
    _RecordPerformanceFrame(event);

    if (event.fixedSteps == 0) {
        return;
    }

    const double elapsedDelta =
        static_cast<double>(event.fixedSteps) * event.fixedStepSeconds;

    for (auto& [_, accumulator] : m_sessions) {
        if (!accumulator.snapshot.active) {
            continue;
        }

        accumulator.snapshot.ElapsedSeconds += elapsedDelta;
        accumulator.elapsedSinceLastPublish += elapsedDelta;
        _RefreshDerivedMetrics(accumulator);

        if (accumulator.elapsedSinceLastPublish >=
            REALTIME_PUBLISH_INTERVAL_SECONDS) {
            _PublishRealtime(accumulator);
            accumulator.elapsedSinceLastPublish = 0.0;
        }
    }
}

void StatTracker::_RecordPerformanceFrame(
    const core::events::FrameTickEvent& event) {
    if (event.frameSeconds <= 0.0) {
        return;
    }

    const double frameTimeMs = event.frameSeconds * 1000.0;
    m_performanceAccumulator.windowSeconds += event.frameSeconds;
    m_performanceAccumulator.sampledFrames += 1;
    m_performanceAccumulator.frameTimeMsSum += frameTimeMs;
    m_performanceAccumulator.frameTimeSquaredMsSum += frameTimeMs * frameTimeMs;
    m_performanceAccumulator.simulationSecondsSum +=
        static_cast<double>(event.fixedSteps) * event.fixedStepSeconds;
    m_performanceAccumulator.worstFrameTimeMs =
        std::max(m_performanceAccumulator.worstFrameTimeMs, frameTimeMs);

    if (frameTimeMs > TARGET_FRAME_TIME_MS) {
        m_performanceAccumulator.missedTargetFrames += 1;
    }

    m_performanceAccumulator.frameTimesMs.push_back(frameTimeMs);

    if (m_performanceAccumulator.windowSeconds <
            PERFORMANCE_PUBLISH_INTERVAL_SECONDS ||
        m_performanceAccumulator.sampledFrames == 0) {
        return;
    }

    _PublishPerformance(m_performanceAccumulator);
    _ResetPerformanceAccumulator();
}

void StatTracker::_PublishPerformance(
    const PerformanceAccumulator& accumulator) {
    if (accumulator.sampledFrames == 0 || accumulator.windowSeconds <= 0.0) {
        return;
    }

    const double sampledFrames = static_cast<double>(accumulator.sampledFrames);
    const double averageFrameTimeMs = accumulator.frameTimeMsSum / sampledFrames;
    const double variance =
        std::max(0.0, accumulator.frameTimeSquaredMsSum / sampledFrames -
                          averageFrameTimeMs * averageFrameTimeMs);
    const double frameJitterMs = std::sqrt(variance);
    const double fps = sampledFrames / accumulator.windowSeconds;
    const double simulationLoadPercent =
        (accumulator.simulationSecondsSum / accumulator.windowSeconds) * 100.0;
    const double missedTargetFramePercent =
        static_cast<double>(accumulator.missedTargetFrames) * 100.0 /
        sampledFrames;

    // p95 via partial sort (cheap for typical window sizes)
    double p95FrameTimeMs = averageFrameTimeMs;
    if (!accumulator.frameTimesMs.empty()) {
        std::vector<double> frameSamples = accumulator.frameTimesMs;
        const std::size_t sampleCount = frameSamples.size();
        const std::size_t p95Index = static_cast<std::size_t>(std::ceil(
                                         static_cast<double>(sampleCount) * 0.95)) -
                                     1;
        std::nth_element(frameSamples.begin(), frameSamples.begin() + p95Index,
                         frameSamples.end());
        p95FrameTimeMs = frameSamples[p95Index];
    }

    m_eventBus.Publish(core::events::PerformanceStatsEvent{
        accumulator.windowSeconds,
        accumulator.sampledFrames,
        fps,
        averageFrameTimeMs,
        p95FrameTimeMs,
        accumulator.worstFrameTimeMs,
        frameJitterMs,
        simulationLoadPercent,
        missedTargetFramePercent,
    });
}

void StatTracker::_ResetPerformanceAccumulator() {
    m_performanceAccumulator.windowSeconds = 0.0;
    m_performanceAccumulator.sampledFrames = 0;
    m_performanceAccumulator.frameTimeMsSum = 0.0;
    m_performanceAccumulator.frameTimeSquaredMsSum = 0.0;
    m_performanceAccumulator.simulationSecondsSum = 0.0;
    m_performanceAccumulator.worstFrameTimeMs = 0.0;
    m_performanceAccumulator.missedTargetFrames = 0;
    m_performanceAccumulator.frameTimesMs.clear();
}

void StatTracker::_OnShotFired(const core::events::ShotFiredEvent& event) {
    auto* accumulator = _GetSession(event.sessionId);
    if (accumulator == nullptr) {
        return;
    }

    accumulator->snapshot.shotsFired += 1;
    _RefreshDerivedMetrics(*accumulator);
    _PublishRealtime(*accumulator);
}

void StatTracker::_OnShotHit(const core::events::ShotHitEvent& event) {
    auto* accumulator = _GetSession(event.sessionId);
    if (accumulator == nullptr) {
        return;
    }

    accumulator->snapshot.hits += 1;

    // Tracking mode: measure recovery time from first miss to next hit
    double reactionTimeMsForScore = event.reactionTimeMs;
    if (accumulator->trackingMode) {
        if (accumulator->trackingMissPending &&
            event.timestamp > accumulator->trackingFirstMissTimestamp) {
            const double recoveryReactionMs =
                std::chrono::duration<double, std::milli>(
                    event.timestamp - accumulator->trackingFirstMissTimestamp)
                    .count();
            accumulator->reactionSumMs += recoveryReactionMs;
            accumulator->reactionSampleCount += 1;
            accumulator->trackingMissPending = false;
            reactionTimeMsForScore = recoveryReactionMs;
        } else {
            reactionTimeMsForScore = TRACKING_FALLBACK_REACTION_MS;
        }
    } else {
        accumulator->reactionSumMs += event.reactionTimeMs;
        accumulator->reactionSampleCount += 1;
    }

    accumulator->snapshot.score +=
        _HitScoreDelta(*accumulator, reactionTimeMsForScore);
    _RefreshDerivedMetrics(*accumulator);
    _PublishRealtime(*accumulator);
}

void StatTracker::_OnShotMiss(const core::events::ShotMissEvent& event) {
    auto* accumulator = _GetSession(event.sessionId);
    if (accumulator == nullptr) {
        return;
    }

    accumulator->snapshot.misses += 1;
    accumulator->snapshot.score -= _MissScoreDelta(*accumulator);

    if (accumulator->trackingMode && !accumulator->trackingMissPending) {
        accumulator->trackingMissPending = true;
        accumulator->trackingFirstMissTimestamp = event.timestamp;
    }

    _RefreshDerivedMetrics(*accumulator);
    _PublishRealtime(*accumulator);
}

void StatTracker::_OnTargetSpawned(
    const core::events::TargetSpawnedEvent& event) {
    auto* accumulator = _GetSession(event.sessionId);
    if (accumulator == nullptr) {
        return;
    }

    accumulator->snapshot.targetsSpawned += 1;
    _RefreshDerivedMetrics(*accumulator);
    _PublishRealtime(*accumulator);
}

void StatTracker::_OnTargetDestroyed(
    const core::events::TargetDestroyedEvent& event) {
    auto* accumulator = _GetSession(event.sessionId);
    if (accumulator == nullptr) {
        return;
    }

    accumulator->snapshot.targetsDestroyed += 1;
    _RefreshDerivedMetrics(*accumulator);
    _PublishRealtime(*accumulator);
}

void StatTracker::_RefreshDerivedMetrics(SessionAccumulator& accumulator) {
    const auto shotsFired = accumulator.snapshot.shotsFired;
    const auto hits = accumulator.snapshot.hits;

    accumulator.snapshot.accuracyPercent =
        shotsFired > 0
            ? static_cast<double>(hits) * 100.0 / static_cast<double>(shotsFired)
            : 0.0;

    accumulator.snapshot.averageReactionTimeMs =
        accumulator.reactionSampleCount > 0
            ? accumulator.reactionSumMs /
                  static_cast<double>(accumulator.reactionSampleCount)
            : 0.0;

    accumulator.snapshot.shotsPerSecond =
        accumulator.snapshot.ElapsedSeconds > 0.0
            ? static_cast<double>(shotsFired) /
                  accumulator.snapshot.ElapsedSeconds
            : 0.0;
}

void StatTracker::_PublishRealtime(SessionAccumulator& accumulator) {
    m_eventBus.Publish(core::events::RealtimeStatsEvent{
        accumulator.snapshot.SessionId, accumulator.snapshot.modeId,
        accumulator.snapshot.ElapsedSeconds, accumulator.snapshot.score,
        accumulator.snapshot.shotsFired, accumulator.snapshot.hits,
        accumulator.snapshot.misses, accumulator.snapshot.targetsSpawned,
        accumulator.snapshot.targetsDestroyed,
        accumulator.snapshot.accuracyPercent,
        accumulator.snapshot.averageReactionTimeMs,
        accumulator.snapshot.shotsPerSecond});
}

// Combines distance, radius, mode-specific and grid-density factors.
double StatTracker::_ScoreMultiplierForSession(
    const core::events::SessionStartedEvent& event) {
    const double distanceMultiplier =
        std::max(0.1, event.configuredDistanceUnits) / 10.0;

    const double clampedRadius = std::max(0.05, event.targetRadius);
    const double radiusMultiplier = std::clamp(0.5 / clampedRadius, 0.25, 8.0);

    double modeMultiplier = 1.0;

    if (event.modeId == "next_shot") {
        modeMultiplier *= 0.75;
    }

    if (event.modeId == "tracking_targets" ||
        event.modeId == "strafing_targets") {
        const double speed = ModeSettingValue(event.modeSettings, "speed", 2.5);
        modeMultiplier *= std::max(0.1, speed) / 2.5;
    }

    if (event.modeId == "gridshot") {
        const int gridRows =
            std::max(2, static_cast<int>(std::lround(ModeSettingValue(
                            event.modeSettings, "grid_rows", 5.0))));
        const int gridColumns =
            std::max(2, static_cast<int>(std::lround(ModeSettingValue(
                            event.modeSettings, "grid_columns", 5.0))));
        const int activeTargets =
            std::max(1, static_cast<int>(std::lround(ModeSettingValue(
                            event.modeSettings, "active_target_count", 3.0))));

        const int totalCells = std::max(1, gridRows * gridColumns);
        const double occupancyMultiplier =
            static_cast<double>(totalCells) / static_cast<double>(activeTargets);
        modeMultiplier *= std::clamp(occupancyMultiplier, 1.0, 30.0);
    }

    return std::clamp(distanceMultiplier * radiusMultiplier * modeMultiplier, 0.1,
                      120.0);
}

// Faster reaction -> higher multiplier (hyperbolic curve).
std::int64_t StatTracker::_HitScoreDelta(const SessionAccumulator& accumulator,
                                         double reactionTimeMs) {
    const double safeReactionTimeMs = std::max(1.0, reactionTimeMs);
    const double reactionMultiplier =
        std::clamp(350.0 / (safeReactionTimeMs + 150.0), 0.35, 1.75);
    const double rawScoreDelta =
        BASE_HIT_POINTS * accumulator.scoreMultiplier * reactionMultiplier;
    return std::max<std::int64_t>(
        1, static_cast<std::int64_t>(std::llround(rawScoreDelta)));
}

std::int64_t StatTracker::_MissScoreDelta(const SessionAccumulator& accumulator) {
    const double rawPenalty = BASE_MISS_POINTS * accumulator.scoreMultiplier;
    return std::max<std::int64_t>(
        1, static_cast<std::int64_t>(std::llround(rawPenalty)));
}

void StatTracker::_PublishSummary(const SessionAccumulator& accumulator) {
    m_eventBus.Publish(core::events::SessionSummaryEvent{
        accumulator.snapshot.SessionId, accumulator.snapshot.modeId,
        accumulator.snapshot.ElapsedSeconds, accumulator.snapshot.score,
        accumulator.snapshot.shotsFired, accumulator.snapshot.hits,
        accumulator.snapshot.misses, accumulator.snapshot.targetsSpawned,
        accumulator.snapshot.targetsDestroyed,
        accumulator.snapshot.accuracyPercent,
        accumulator.snapshot.averageReactionTimeMs,
        accumulator.snapshot.shotsPerSecond});
}

StatTracker::SessionAccumulator* StatTracker::_GetSession(std::uint64_t SessionId) {
    const auto iterator = m_sessions.find(SessionId);
    if (iterator == m_sessions.end()) {
        return nullptr;
    }

    return &iterator->second;
}

const StatTracker::SessionAccumulator* StatTracker::_GetSession(std::uint64_t SessionId) const {
    const auto iterator = m_sessions.find(SessionId);
    if (iterator == m_sessions.end()) {
        return nullptr;
    }

    return &iterator->second;
}
}  // namespace xaimassist::stats
