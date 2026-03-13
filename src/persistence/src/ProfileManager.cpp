/// @file ProfileManager.cpp
#include "persistence/ProfileManager.hpp"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <utility>

#include "core/Logger.hpp"
#include "persistence/PersistenceDatabase.hpp"

namespace xaimassist::persistence {
ProfileManager::ProfileManager(PersistenceDatabase& database,
                               core::Logger& logger)
    : m_database(database), m_logger(logger) {}

bool ProfileManager::EnsureDefaultProfile() {
    const auto existingProfiles = Profiles();
    if (existingProfiles.empty()) {
        const auto profileId = CreateProfile("Default");
        if (profileId <= 0) {
            return false;
        }

        return SetActiveProfile(profileId);
    }

    if (ActiveProfileId() > 0) {
        return true;
    }

    return SetActiveProfile(existingProfiles.front().id);
}

std::int64_t ProfileManager::CreateProfile(const std::string& profileName) {
    QSqlDatabase database = m_database.Connection();
    if (!database.isOpen()) {
        return 0;
    }

    QSqlQuery query(database);
    query.prepare(
        "INSERT INTO Profiles(name, created_at, updated_at, is_active) "
        "VALUES(:name, :createdAt, :updatedAt, 0);");

    const QString timestamp = QString::fromStdString(_NowIsoUtc());
    query.bindValue(":name", QString::fromStdString(profileName));
    query.bindValue(":createdAt", timestamp);
    query.bindValue(":updatedAt", timestamp);

    if (!query.exec()) {
        m_logger.Error("persistence", "Failed to create profile");
        return 0;
    }

    return query.lastInsertId().toLongLong();
}

bool ProfileManager::SetActiveProfile(std::int64_t profileId) {
    if (profileId <= 0) {
        return false;
    }

    QSqlDatabase database = m_database.Connection();
    if (!database.isOpen()) {
        return false;
    }

    if (!database.transaction()) {
        return false;
    }

    QSqlQuery clearActive(database);
    if (!clearActive.exec("UPDATE Profiles SET is_active = 0;")) {
        database.rollback();
        return false;
    }

    QSqlQuery setActive(database);
    setActive.prepare(
        "UPDATE Profiles "
        "SET is_active = 1, updated_at = :updatedAt "
        "WHERE id = :id;");
    setActive.bindValue(":updatedAt", QString::fromStdString(_NowIsoUtc()));
    setActive.bindValue(":id", static_cast<qlonglong>(profileId));

    if (!setActive.exec() || setActive.numRowsAffected() <= 0) {
        database.rollback();
        return false;
    }

    return database.commit();
}

std::int64_t ProfileManager::ActiveProfileId() const {
    QSqlDatabase database = m_database.Connection();
    if (!database.isOpen()) {
        return 0;
    }

    QSqlQuery query(database);
    if (!query.exec("SELECT id FROM Profiles WHERE is_active = 1 LIMIT 1;") ||
        !query.next()) {
        return 0;
    }

    return query.value(0).toLongLong();
}

std::optional<ProfileRecord> ProfileManager::ActiveProfile() const {
    QSqlDatabase database = m_database.Connection();
    if (!database.isOpen()) {
        return std::nullopt;
    }

    QSqlQuery query(database);
    if (!query.exec("SELECT id, name, created_at, updated_at, is_active "
                    "FROM Profiles "
                    "WHERE is_active = 1 "
                    "LIMIT 1;") ||
        !query.next()) {
        return std::nullopt;
    }

    ProfileRecord record;
    record.id = query.value(0).toLongLong();
    record.name = query.value(1).toString().toStdString();
    record.createdAtIso = query.value(2).toString().toStdString();
    record.updatedAtIso = query.value(3).toString().toStdString();
    record.active = query.value(4).toInt() != 0;
    return record;
}

std::vector<ProfileRecord> ProfileManager::Profiles() const {
    std::vector<ProfileRecord> result;

    QSqlDatabase database = m_database.Connection();
    if (!database.isOpen()) {
        return result;
    }

    QSqlQuery query(database);
    if (!query.exec("SELECT id, name, created_at, updated_at, is_active "
                    "FROM Profiles "
                    "ORDER BY id ASC;")) {
        return result;
    }

    while (query.next()) {
        ProfileRecord record;
        record.id = query.value(0).toLongLong();
        record.name = query.value(1).toString().toStdString();
        record.createdAtIso = query.value(2).toString().toStdString();
        record.updatedAtIso = query.value(3).toString().toStdString();
        record.active = query.value(4).toInt() != 0;
        result.push_back(std::move(record));
    }

    return result;
}

std::string ProfileManager::_NowIsoUtc() {
    return QDateTime::currentDateTimeUtc()
        .toString(Qt::ISODateWithMs)
        .toStdString();
}
}  // namespace xaimassist::persistence
