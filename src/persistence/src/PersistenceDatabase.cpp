/// @file PersistenceDatabase.cpp
#include "persistence/PersistenceDatabase.hpp"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>
#include <atomic>
#include <filesystem>

#include "core/Logger.hpp"

namespace {
constexpr int CURRENT_SCHEMA_VERSION = 7;

std::atomic<std::uint64_t> g_connectionCounter{1};

const xaimassist::core::Logger& fallbackLogger() {
    static const xaimassist::core::Logger logger;
    return logger;
}
}  // namespace

namespace xaimassist::persistence {
PersistenceDatabase::PersistenceDatabase(const core::Logger* logger,
                                         std::string DatabasePath)
    : m_logger(logger != nullptr ? logger : &fallbackLogger()),
      m_databasePath(DatabasePath.empty() ? DefaultDatabasePath(m_logger)
                                          : std::move(DatabasePath)),
      m_connectionName("xaimassist_persistence_" +
                       std::to_string(g_connectionCounter.fetch_add(1))) {}

PersistenceDatabase::~PersistenceDatabase() {
    if (m_database != nullptr) {
        const QString connectionName = m_database->connectionName();
        _LogInfo("Closing SQLite connection: " + connectionName.toStdString());
        m_database->close();
        m_database.reset();
        QSqlDatabase::removeDatabase(connectionName);
    }
}

bool PersistenceDatabase::Initialize() {
    if (m_initialized) {
        _LogInfo("Initialize called on already initialized persistence database");
        return true;
    }

    _LogInfo("Initializing SQLite persistence database at: " + m_databasePath);

    if (!_Open()) {
        _LogError("Failed to open SQLite persistence database");
        return false;
    }

    if (!_EnsureSchema()) {
        _LogError("Failed to ensure persistence schema");
        return false;
    }

    m_initialized = true;
    _LogInfo("SQLite persistence database initialized successfully");
    return true;
}

QSqlDatabase PersistenceDatabase::Connection() const {
    if (m_database == nullptr) {
        return QSqlDatabase{};
    }

    return *m_database;
}

const std::string& PersistenceDatabase::DatabasePath() const noexcept {
    return m_databasePath;
}

std::string
PersistenceDatabase::DefaultDatabasePath(const core::Logger* logger) {
    const core::Logger& activeLogger =
        logger != nullptr ? *logger : fallbackLogger();

    const QString appDataLocation =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!appDataLocation.isEmpty()) {
        const std::string databasePath =
            (std::filesystem::path(appDataLocation.toStdString()) /
             "xaimassist.sqlite")
                .string();
        activeLogger.Info("persistence",
                          "Default database path resolved: " + databasePath);
        return databasePath;
    }

    std::error_code error;
    const std::filesystem::path fallbackDirectory =
        std::filesystem::temp_directory_path(error);
    if (error) {
        activeLogger.Warning(
            "persistence",
            "Failed to resolve AppDataLocation and temp directory; "
            "using relative database path: xaimassist.sqlite");
        return "xaimassist.sqlite";
    }

    const std::string databasePath =
        (fallbackDirectory / "xaimassist.sqlite").string();
    activeLogger.Warning(
        "persistence",
        "AppDataLocation unavailable; using temp database path: " + databasePath);
    return databasePath;
}

bool PersistenceDatabase::_Open() {
    _LogInfo("Opening SQLite database connection: " + m_connectionName);

    const std::filesystem::path DatabasePath(m_databasePath);
    if (DatabasePath.has_parent_path()) {
        std::error_code Error;
        std::filesystem::create_directories(DatabasePath.parent_path(), Error);
        if (Error) {
            _LogError("Failed to create database directory: " +
                      DatabasePath.parent_path().string() +
                      " | error: " + Error.message());
            return false;
        }
    }

    auto database = QSqlDatabase::addDatabase(
        "QSQLITE", QString::fromStdString(m_connectionName));
    if (!database.isValid()) {
        _LogError("QSQLITE driver is not available for connection: " +
                  m_connectionName);
        return false;
    }

    database.setDatabaseName(QString::fromStdString(m_databasePath));
    if (!database.open()) {
        _LogError("Failed to open database file: " + m_databasePath +
                  " | error: " + database.lastError().text().toStdString());
        return false;
    }

    m_database = std::make_unique<QSqlDatabase>(database);
    if (!_ExecuteStatement("PRAGMA foreign_keys = ON;")) {
        _LogError("Failed to enable SQLite foreign key enforcement");
        return false;
    }

    _LogInfo("SQLite database connection opened successfully");
    return true;
}

bool PersistenceDatabase::_EnsureSchema() {
    _LogInfo("Ensuring persistence schema state");

    if (!_ExecuteStatement("CREATE TABLE IF NOT EXISTS schema_version (version "
                           "INTEGER NOT NULL);") ||
        !_ExecuteStatement(
            "INSERT INTO schema_version(version) SELECT 0 WHERE "
            "NOT EXISTS (SELECT 1 FROM schema_version LIMIT 1);")) {
        return false;
    }

    int version = _SchemaVersion();
    if (version < 0) {
        _LogError("Failed to read schema version");
        return false;
    }

    if (version < 1) {
        _LogInfo("Applying schema migration to version 1");
        if (!_ExecuteStatement("CREATE TABLE IF NOT EXISTS Profiles ("
                               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                               "name TEXT NOT NULL UNIQUE,"
                               "created_at TEXT NOT NULL,"
                               "updated_at TEXT NOT NULL,"
                               "is_active INTEGER NOT NULL DEFAULT 0"
                               ");") ||
            !_ExecuteStatement(
                "CREATE TABLE IF NOT EXISTS profile_settings ("
                "profile_id INTEGER PRIMARY KEY,"
                "raw_input_enabled INTEGER NOT NULL,"
                "cm_per_360 REAL NOT NULL,"
                "Dpi REAL NOT NULL,"
                "sensitivity_scale REAL NOT NULL,"
                "yaw_multiplier REAL NOT NULL,"
                "pitch_multiplier REAL NOT NULL,"
                "scoped_multiplier REAL NOT NULL,"
                "invert_y INTEGER NOT NULL,"
                "target_color_r REAL NOT NULL,"
                "target_color_g REAL NOT NULL,"
                "target_color_b REAL NOT NULL,"
                "target_radius REAL NOT NULL,"
                "updated_at TEXT NOT NULL,"
                "FOREIGN KEY(profile_id) REFERENCES Profiles(id) ON DELETE CASCADE"
                ");") ||
            !_ExecuteStatement(
                "CREATE TABLE IF NOT EXISTS session_history ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "profile_id INTEGER NOT NULL,"
                "runtime_session_id INTEGER NOT NULL,"
                "mode_id TEXT NOT NULL,"
                "started_at TEXT NOT NULL,"
                "stopped_at TEXT NOT NULL,"
                "elapsed_seconds REAL NOT NULL,"
                "score INTEGER NOT NULL,"
                "shots_fired INTEGER NOT NULL,"
                "hits INTEGER NOT NULL,"
                "misses INTEGER NOT NULL,"
                "targets_spawned INTEGER NOT NULL,"
                "targets_destroyed INTEGER NOT NULL,"
                "accuracy_percent REAL NOT NULL,"
                "average_reaction_time_ms REAL NOT NULL,"
                "shots_per_second REAL NOT NULL,"
                "created_at TEXT NOT NULL,"
                "FOREIGN KEY(profile_id) REFERENCES Profiles(id) ON DELETE CASCADE"
                ");") ||
            !_ExecuteStatement(
                "CREATE INDEX IF NOT EXISTS idx_session_history_profile_created "
                "ON session_history(profile_id, created_at DESC);") ||
            !_ExecuteStatement(
                "CREATE INDEX IF NOT EXISTS idx_session_history_profile_mode_score "
                "ON session_history(profile_id, mode_id, score DESC);") ||
            !_SetSchemaVersion(1)) {
            return false;
        }

        version = 1;
    }

    if (version < 2) {
        _LogInfo("Applying schema migration to version 2");
        if (!_TableHasColumn("profile_settings", "theme_mode") &&
            !_ExecuteStatement(
                "ALTER TABLE profile_settings "
                "ADD COLUMN theme_mode TEXT NOT NULL DEFAULT 'dark';")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "language") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN language TEXT NOT NULL DEFAULT 'en';")) {
            return false;
        }

        if (!_SetSchemaVersion(2)) {
            return false;
        }

        version = 2;
    }

    if (version < 3) {
        _LogInfo("Applying schema migration to version 3");
        if (!_TableHasColumn("profile_settings", "crosshair_thickness") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_thickness REAL NOT NULL "
                               "DEFAULT 2.0;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "crosshair_center_dot_enabled") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_center_dot_enabled INTEGER "
                               "NOT NULL DEFAULT 1;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "crosshair_center_dot_size") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_center_dot_size REAL NOT "
                               "NULL DEFAULT 4.0;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "crosshair_color_r") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_color_r REAL NOT NULL "
                               "DEFAULT 0.95;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "crosshair_color_g") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_color_g REAL NOT NULL "
                               "DEFAULT 0.97;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "crosshair_color_b") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_color_b REAL NOT NULL "
                               "DEFAULT 0.99;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "crosshair_border_enabled") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_border_enabled INTEGER "
                               "NOT NULL DEFAULT 1;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "crosshair_border_thickness") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_border_thickness REAL NOT "
                               "NULL DEFAULT 1.0;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "crosshair_horizontal_length") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_horizontal_length REAL NOT "
                               "NULL DEFAULT 10.0;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "crosshair_vertical_length") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_vertical_length REAL NOT "
                               "NULL DEFAULT 10.0;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "crosshair_gap") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_gap REAL NOT NULL "
                               "DEFAULT 4.0;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "fps_enabled") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN fps_enabled INTEGER NOT NULL "
                               "DEFAULT 0;")) {
            return false;
        }

        if (!_TableHasColumn("profile_settings", "fps_position") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN fps_position TEXT NOT NULL "
                               "DEFAULT 'top_right';")) {
            return false;
        }

        if (!_SetSchemaVersion(3)) {
            return false;
        }

        version = 3;
    }

    if (version < 4) {
        _LogInfo("Applying schema migration to version 4");
        if (!_TableHasColumn("profile_settings", "crosshair_lines_enabled") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN crosshair_lines_enabled INTEGER "
                               "NOT NULL DEFAULT 1;")) {
            return false;
        }

        if (!_SetSchemaVersion(4)) {
            return false;
        }

        version = 4;
    }

    if (version < 5) {
        _LogInfo("Applying schema migration to version 5");
        if (!_TableHasColumn("profile_settings", "sensitivity_scale") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN sensitivity_scale REAL NOT NULL "
                               "DEFAULT 1.0;")) {
            return false;
        }

        if (!_SetSchemaVersion(5)) {
            return false;
        }

        version = 5;
    }

    if (version < 6) {
        _LogInfo("Applying schema migration to version 6");
        if (!_TableHasColumn("profile_settings", "selected_mode_id") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN selected_mode_id TEXT NOT NULL "
                               "DEFAULT 'static_sphere';")) {
            return false;
        }

        if (!_SetSchemaVersion(6)) {
            return false;
        }

        version = 6;
    }

    if (version < 7) {
        _LogInfo("Applying schema migration to version 7");
        if (!_TableHasColumn("profile_settings", "mode_overrides_json") &&
            !_ExecuteStatement("ALTER TABLE profile_settings "
                               "ADD COLUMN mode_overrides_json TEXT NOT NULL "
                               "DEFAULT '{}';")) {
            return false;
        }

        if (!_SetSchemaVersion(7)) {
            return false;
        }

        version = 7;
    }

    if (version < CURRENT_SCHEMA_VERSION) {
        _LogError("Persistence schema is behind expected version. Current: " +
                  std::to_string(version) +
                  " | Expected: " + std::to_string(CURRENT_SCHEMA_VERSION));
        return false;
    }

    _LogInfo("Persistence schema ready at version " + std::to_string(version));
    return true;
}

bool PersistenceDatabase::_ExecuteStatement(const char* sql) const {
    if (m_database == nullptr) {
        _LogError("Attempted to execute SQL statement without an open database");
        return false;
    }

    if (sql == nullptr) {
        _LogError("Attempted to execute null SQL statement");
        return false;
    }

    QSqlQuery query(*m_database);
    if (!query.exec(QString::fromUtf8(sql))) {
        _LogError("SQL statement failed: " + std::string(sql) +
                  " | error: " + query.lastError().text().toStdString());
        return false;
    }

    return true;
}

bool PersistenceDatabase::_TableHasColumn(const char* tableName,
                                          const char* columnName) const {
    if (m_database == nullptr || tableName == nullptr || columnName == nullptr) {
        _LogWarning(
            "Unable to inspect table column because database/table/column "
            "is invalid");
        return false;
    }

    QSqlQuery query(*m_database);
    if (!query.exec(QString::fromUtf8("PRAGMA table_info(%1);")
                        .arg(QString::fromUtf8(tableName)))) {
        _LogError("Failed to inspect table columns for table '" +
                  std::string(tableName) +
                  "' | error: " + query.lastError().text().toStdString());
        return false;
    }

    const QString expectedColumnName = QString::fromUtf8(columnName);
    while (query.next()) {
        if (query.value(1).toString().compare(expectedColumnName,
                                              Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

int PersistenceDatabase::_SchemaVersion() const {
    if (m_database == nullptr) {
        _LogError("Cannot query schema version without an open database");
        return -1;
    }

    QSqlQuery query(*m_database);
    if (!query.exec("SELECT version FROM schema_version LIMIT 1;")) {
        _LogError("Failed to query schema version: " +
                  query.lastError().text().toStdString());
        return -1;
    }

    if (!query.next()) {
        _LogError("schema_version table is empty");
        return -1;
    }

    return query.value(0).toInt();
}

bool PersistenceDatabase::_SetSchemaVersion(int version) const {
    if (m_database == nullptr) {
        _LogError("Cannot update schema version without an open database");
        return false;
    }

    QSqlQuery query(*m_database);
    if (!query.exec("DELETE FROM schema_version;")) {
        _LogError("Failed to clear schema_version table: " +
                  query.lastError().text().toStdString());
        return false;
    }

    query.prepare("INSERT INTO schema_version(version) VALUES(:version);");
    query.bindValue(":version", version);
    if (!query.exec()) {
        _LogError("Failed to set schema version to " + std::to_string(version) +
                  " | error: " + query.lastError().text().toStdString());
        return false;
    }

    _LogInfo("Schema version updated to " + std::to_string(version));
    return true;
}

void PersistenceDatabase::_LogInfo(const std::string& message) const {
    if (m_logger != nullptr) {
        m_logger->Info("persistence", message);
    }
}

void PersistenceDatabase::_LogWarning(const std::string& message) const {
    if (m_logger != nullptr) {
        m_logger->Warning("persistence", message);
    }
}

void PersistenceDatabase::_LogError(const std::string& message) const {
    if (m_logger != nullptr) {
        m_logger->Error("persistence", message);
    }
}
}  // namespace xaimassist::persistence
