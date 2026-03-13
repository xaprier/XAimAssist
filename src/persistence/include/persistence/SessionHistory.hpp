/**
 * @file SessionHistory.hpp
 * @brief Persists completed training sessions and provides query helpers.
 */

#ifndef SESSIONHISTORY_HPP
#define SESSIONHISTORY_HPP

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"

class QSqlQuery;

namespace xaimassist::core {
class Logger;
}

namespace xaimassist::persistence {
class PersistenceDatabase;
class ProfileManager;

/// Flat row representation of a persisted session.
struct SessionRecord {
    std::int64_t id{0};
    std::int64_t profileId{0};
    std::uint64_t runtimeSessionId{0};
    std::string modeId;
    std::string startedAtIso;
    std::string stoppedAtIso;

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
    std::string createdAtIso;
};

/// Aggregated min/avg/max for a single metric across sessions.
struct ModeMetricAggregate {
    double average{0.0};
    double minimum{0.0};
    double maximum{0.0};
};

/// Per-mode statistical summary.
struct ModePerformanceSummary {
    std::string modeId;
    std::uint64_t sessionCount{0};
    ModeMetricAggregate score;
    ModeMetricAggregate hits;
    ModeMetricAggregate misses;
    ModeMetricAggregate accuracyPercent;
    ModeMetricAggregate averageReactionTimeMs;
    ModeMetricAggregate shotsPerSecond;
};

/// Outcome of validating a session before persistence.
struct SessionValidationResult {
    bool valid{true};
    std::string reasonCode;
};

/**
 * @class SessionHistory
 * @brief Listens for session events, persists summaries, and provides
 *        queries for recent/best sessions and per-mode aggregates.
 */
class SessionHistory {
  public:
    SessionHistory(PersistenceDatabase& database, ProfileManager& profileManager,
                   core::EventBus& eventBus, core::Logger& logger);
    ~SessionHistory();

    /// Fetch the most recent sessions for a profile (newest first).
    std::vector<SessionRecord> GetRecentSessions(std::int64_t profileId,
                                                 int limit = 30) const;

    /// Return the highest-score session for the given mode, if any.
    std::optional<SessionRecord> GetBestSessionForMode(std::int64_t profileId,
                                                       const std::string& modeId) const;

    /// Compute aggregate statistics across all sessions of a mode.
    std::optional<ModePerformanceSummary> GetModePerformanceSummary(std::int64_t profileId,
                                                                    const std::string& modeId) const;

    /// Check whether a session summary is valid for persistence.
    static SessionValidationResult ValidateSessionSummary(const core::events::SessionSummaryEvent& event);

  private:
    struct RuntimeSessionContext {
        std::string modeId;
        std::chrono::system_clock::time_point startedAt;
        std::chrono::system_clock::time_point stoppedAt;
        core::events::SessionStopReason stopReason{core::events::SessionStopReason::Completed};
        bool hasStarted{false};
        bool hasStopped{false};
    };

    void _HandleCoreEvent(const core::events::CoreEvent& event);
    void _OnSessionStarted(const core::events::SessionStartedEvent& event);
    void _OnSessionStopped(const core::events::SessionStoppedEvent& event);
    void _OnSessionSummary(const core::events::SessionSummaryEvent& event);

    bool _PersistSessionSummary(std::int64_t profileId,
                                const core::events::SessionSummaryEvent& summaryEvent,
                                const RuntimeSessionContext& runtimeContext);

    static std::string _ToIsoUtc(std::chrono::system_clock::time_point timePoint);
    static SessionRecord _ReadSessionRecordFromQuery(QSqlQuery& query);

    PersistenceDatabase& m_database;
    ProfileManager& m_profileManager;
    core::EventBus& m_eventBus;
    core::Logger& m_logger;
    core::EventBus::SubscriptionId m_subscriptionId{0};
    std::unordered_map<std::uint64_t, RuntimeSessionContext> m_runtimeSessions;
};
}  // namespace xaimassist::persistence

#endif  // SESSIONHISTORY_HPP
