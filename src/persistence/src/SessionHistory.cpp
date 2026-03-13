/// @file SessionHistory.cpp
#include "persistence/SessionHistory.hpp"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimeZone>
#include <QVariant>

#include "core/EventBus.hpp"
#include "core/Logger.hpp"
#include "persistence/PersistenceDatabase.hpp"
#include "persistence/ProfileManager.hpp"

namespace {
constexpr double ACCURACY_EXTREME_EPSILON = 1e-6;
}

namespace xaimassist::persistence {
SessionHistory::SessionHistory(PersistenceDatabase& database,
                               ProfileManager& profileManager,
                               core::EventBus& eventBus, core::Logger& logger)
    : m_database(database), m_profileManager(profileManager), m_eventBus(eventBus), m_logger(logger) {
    m_subscriptionId =
        m_eventBus.Subscribe([this](const core::events::CoreEvent& event) {
            _HandleCoreEvent(event);
        });
}

SessionHistory::~SessionHistory() {
    if (m_subscriptionId != 0) {
        m_eventBus.Unsubscribe(m_subscriptionId);
        m_subscriptionId = 0;
    }
}

std::vector<SessionRecord>
SessionHistory::GetRecentSessions(std::int64_t profileId, int limit) const {
    std::vector<SessionRecord> result;
    if (profileId <= 0 || limit <= 0) {
        return result;
    }

    QSqlDatabase database = m_database.Connection();
    if (!database.isOpen()) {
        return result;
    }

    QSqlQuery query(database);
    query.prepare(
        "SELECT id, profile_id, runtime_session_id, mode_id, "
        "started_at, stopped_at, elapsed_seconds, "
        "score, shots_fired, hits, misses, targets_spawned, "
        "targets_destroyed, accuracy_percent, "
        "average_reaction_time_ms, shots_per_second, created_at "
        "FROM session_history "
        "WHERE profile_id = :profileId "
        "ORDER BY created_at DESC "
        "LIMIT :limit;");
    query.bindValue(":profileId", static_cast<qlonglong>(profileId));
    query.bindValue(":limit", limit);

    if (!query.exec()) {
        return result;
    }

    while (query.next()) {
        result.push_back(_ReadSessionRecordFromQuery(query));
    }

    return result;
}

std::optional<SessionRecord>
SessionHistory::GetBestSessionForMode(std::int64_t profileId,
                                      const std::string& modeId) const {
    if (profileId <= 0 || modeId.empty()) {
        return std::nullopt;
    }

    QSqlDatabase database = m_database.Connection();
    if (!database.isOpen()) {
        return std::nullopt;
    }

    QSqlQuery query(database);
    query.prepare(
        "SELECT id, profile_id, runtime_session_id, mode_id, "
        "started_at, stopped_at, elapsed_seconds, "
        "score, shots_fired, hits, misses, targets_spawned, "
        "targets_destroyed, accuracy_percent, "
        "average_reaction_time_ms, shots_per_second, created_at "
        "FROM session_history "
        "WHERE profile_id = :profileId AND mode_id = :modeId "
        "ORDER BY score DESC, accuracy_percent DESC, created_at DESC "
        "LIMIT 1;");
    query.bindValue(":profileId", static_cast<qlonglong>(profileId));
    query.bindValue(":modeId", QString::fromStdString(modeId));

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return _ReadSessionRecordFromQuery(query);
}

std::optional<ModePerformanceSummary>
SessionHistory::GetModePerformanceSummary(std::int64_t profileId,
                                          const std::string& modeId) const {
    if (profileId <= 0 || modeId.empty()) {
        return std::nullopt;
    }

    QSqlDatabase database = m_database.Connection();
    if (!database.isOpen()) {
        return std::nullopt;
    }

    QSqlQuery query(database);
    query.prepare(
        "SELECT COUNT(*), "
        "AVG(score), MIN(score), MAX(score), "
        "AVG(hits), MIN(hits), MAX(hits), "
        "AVG(misses), MIN(misses), MAX(misses), "
        "AVG(accuracy_percent), MIN(accuracy_percent), "
        "MAX(accuracy_percent), "
        "AVG(average_reaction_time_ms), "
        "MIN(average_reaction_time_ms), "
        "MAX(average_reaction_time_ms), "
        "AVG(shots_per_second), MIN(shots_per_second), "
        "MAX(shots_per_second) "
        "FROM session_history "
        "WHERE profile_id = :profileId AND mode_id = :modeId;");
    query.bindValue(":profileId", static_cast<qlonglong>(profileId));
    query.bindValue(":modeId", QString::fromStdString(modeId));

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    const auto sessionCount =
        static_cast<std::uint64_t>(query.value(0).toLongLong());
    if (sessionCount == 0) {
        return std::nullopt;
    }

    ModePerformanceSummary summary;
    summary.modeId = modeId;
    summary.sessionCount = sessionCount;

    const auto readAggregate = [&](int averageIndex, int minimumIndex,
                                   int maximumIndex) -> ModeMetricAggregate {
        ModeMetricAggregate aggregate;
        aggregate.average = query.value(averageIndex).toDouble();
        aggregate.minimum = query.value(minimumIndex).toDouble();
        aggregate.maximum = query.value(maximumIndex).toDouble();
        return aggregate;
    };

    summary.score = readAggregate(1, 2, 3);
    summary.hits = readAggregate(4, 5, 6);
    summary.misses = readAggregate(7, 8, 9);
    summary.accuracyPercent = readAggregate(10, 11, 12);
    summary.averageReactionTimeMs = readAggregate(13, 14, 15);
    summary.shotsPerSecond = readAggregate(16, 17, 18);

    return summary;
}

SessionValidationResult SessionHistory::ValidateSessionSummary(
    const core::events::SessionSummaryEvent& event) {
    if (event.averageReactionTimeMs <= 0.0) {
        return SessionValidationResult{false, "reaction_time_zero"};
    }

    if (event.hits == 0) {
        return SessionValidationResult{false, "hits_zero"};
    }

    if (event.score < 0) {
        return SessionValidationResult{false, "score_negative"};
    }

    if (event.accuracyPercent <= ACCURACY_EXTREME_EPSILON ||
        event.accuracyPercent > (100.0 + ACCURACY_EXTREME_EPSILON)) {
        return SessionValidationResult{false, "accuracy_extreme"};
    }

    return SessionValidationResult{};
}

void SessionHistory::_HandleCoreEvent(const core::events::CoreEvent& event) {
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

    if (const auto* sessionSummaryEvent =
            std::get_if<core::events::SessionSummaryEvent>(&event)) {
        _OnSessionSummary(*sessionSummaryEvent);
    }
}

void SessionHistory::_OnSessionStarted(
    const core::events::SessionStartedEvent& event) {
    RuntimeSessionContext& context = m_runtimeSessions[event.sessionId];
    context.modeId = event.modeId;
    context.startedAt = event.startedAt;
    context.hasStarted = true;
}

void SessionHistory::_OnSessionStopped(
    const core::events::SessionStoppedEvent& event) {
    RuntimeSessionContext& context = m_runtimeSessions[event.sessionId];
    if (context.modeId.empty()) {
        context.modeId = event.modeId;
    }

    context.stoppedAt = event.stoppedAt;
    context.stopReason = event.stopReason;
    context.hasStopped = true;
}

void SessionHistory::_OnSessionSummary(
    const core::events::SessionSummaryEvent& event) {
    const std::int64_t profileId = m_profileManager.ActiveProfileId();
    if (profileId <= 0) {
        m_logger.Warning("persistence",
                         "Skipping session history Save: no active profile");
        return;
    }

    RuntimeSessionContext context;
    auto iterator = m_runtimeSessions.find(event.sessionId);
    if (iterator != m_runtimeSessions.end()) {
        context = iterator->second;
    }

    if (context.modeId.empty()) {
        context.modeId = event.modeId;
    }

    if (!context.hasStarted) {
        context.startedAt = std::chrono::system_clock::now();
        context.hasStarted = true;
    }

    if (!context.hasStopped) {
        context.stoppedAt = std::chrono::system_clock::now();
        context.hasStopped = true;
    }

    if (context.stopReason == core::events::SessionStopReason::AbortedByUser) {
        m_logger.Info("persistence",
                      "Skipping session history Save: training exited by user");
        m_runtimeSessions.erase(event.sessionId);
        return;
    }

    const SessionValidationResult validation = ValidateSessionSummary(event);
    if (!validation.valid) {
        m_logger.Warning("persistence",
                         "Skipping session history Save: invalid summary (reason=" +
                             validation.reasonCode + ")");
        m_runtimeSessions.erase(event.sessionId);
        return;
    }

    if (_PersistSessionSummary(profileId, event, context)) {
        m_logger.Info("persistence", "Session summary persisted to SQLite history");
    } else {
        m_logger.Error("persistence",
                       "Failed to persist session summary to SQLite history");
    }

    m_runtimeSessions.erase(event.sessionId);
}

bool SessionHistory::_PersistSessionSummary(
    std::int64_t profileId,
    const core::events::SessionSummaryEvent& summaryEvent,
    const RuntimeSessionContext& runtimeContext) {
    QSqlDatabase database = m_database.Connection();
    if (!database.isOpen()) {
        return false;
    }

    QSqlQuery query(database);
    query.prepare(
        "INSERT INTO session_history ("
        "profile_id, runtime_session_id, mode_id, started_at, "
        "stopped_at, elapsed_seconds, score, "
        "shots_fired, hits, misses, targets_spawned, "
        "targets_destroyed, accuracy_percent, "
        "average_reaction_time_ms, shots_per_second, created_at"
        ") VALUES ("
        ":profileId, :runtimeSessionId, :modeId, :startedAt, "
        ":stoppedAt, :ElapsedSeconds, :score, "
        ":shotsFired, :hits, :misses, :targetsSpawned, "
        ":targetsDestroyed, :accuracyPercent, "
        ":averageReactionTime, :shotsPerSecond, :createdAt"
        ");");

    query.bindValue(":profileId", static_cast<qlonglong>(profileId));
    query.bindValue(":runtimeSessionId",
                    static_cast<qlonglong>(summaryEvent.sessionId));
    query.bindValue(":modeId", QString::fromStdString(summaryEvent.modeId));
    query.bindValue(":startedAt",
                    QString::fromStdString(_ToIsoUtc(runtimeContext.startedAt)));
    query.bindValue(":stoppedAt",
                    QString::fromStdString(_ToIsoUtc(runtimeContext.stoppedAt)));
    query.bindValue(":ElapsedSeconds", summaryEvent.elapsedSeconds);
    query.bindValue(":score", static_cast<qlonglong>(summaryEvent.score));
    query.bindValue(":shotsFired",
                    static_cast<qlonglong>(summaryEvent.shotsFired));
    query.bindValue(":hits", static_cast<qlonglong>(summaryEvent.hits));
    query.bindValue(":misses", static_cast<qlonglong>(summaryEvent.misses));
    query.bindValue(":targetsSpawned",
                    static_cast<qlonglong>(summaryEvent.targetsSpawned));
    query.bindValue(":targetsDestroyed",
                    static_cast<qlonglong>(summaryEvent.targetsDestroyed));
    query.bindValue(":accuracyPercent", summaryEvent.accuracyPercent);
    query.bindValue(":averageReactionTime", summaryEvent.averageReactionTimeMs);
    query.bindValue(":shotsPerSecond", summaryEvent.shotsPerSecond);
    query.bindValue(":createdAt", QString::fromStdString(_ToIsoUtc(
                                      std::chrono::system_clock::now())));

    return query.exec();
}

std::string
SessionHistory::_ToIsoUtc(std::chrono::system_clock::time_point timePoint) {
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            timePoint.time_since_epoch());
    const qint64 millisecondsSinceEpoch = milliseconds.count();

    return QDateTime::fromMSecsSinceEpoch(millisecondsSinceEpoch, QTimeZone::UTC)
        .toString(Qt::ISODateWithMs)
        .toStdString();
}

SessionRecord SessionHistory::_ReadSessionRecordFromQuery(QSqlQuery& query) {
    SessionRecord record;
    record.id = query.value(0).toLongLong();
    record.profileId = query.value(1).toLongLong();
    record.runtimeSessionId =
        static_cast<std::uint64_t>(query.value(2).toLongLong());
    record.modeId = query.value(3).toString().toStdString();
    record.startedAtIso = query.value(4).toString().toStdString();
    record.stoppedAtIso = query.value(5).toString().toStdString();
    record.elapsedSeconds = query.value(6).toDouble();
    record.score = query.value(7).toLongLong();
    record.shotsFired = static_cast<std::uint64_t>(query.value(8).toLongLong());
    record.hits = static_cast<std::uint64_t>(query.value(9).toLongLong());
    record.misses = static_cast<std::uint64_t>(query.value(10).toLongLong());
    record.targetsSpawned =
        static_cast<std::uint64_t>(query.value(11).toLongLong());
    record.targetsDestroyed =
        static_cast<std::uint64_t>(query.value(12).toLongLong());
    record.accuracyPercent = query.value(13).toDouble();
    record.averageReactionTimeMs = query.value(14).toDouble();
    record.shotsPerSecond = query.value(15).toDouble();
    record.createdAtIso = query.value(16).toString().toStdString();
    return record;
}
}  // namespace xaimassist::persistence
