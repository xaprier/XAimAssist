/**
 * @file PersistenceDatabase.hpp
 * @brief SQLite database lifecycle and schema migration.
 */

#ifndef PERSISTENCEDATABASE_HPP
#define PERSISTENCEDATABASE_HPP

#include <memory>
#include <string>

class QSqlDatabase;
namespace xaimassist::core {
class Logger;
}

namespace xaimassist::persistence {

/**
 * @class PersistenceDatabase
 * @brief Opens the SQLite database, ensures schema tables exist,
 *        and runs migration steps when the schema version advances.
 */
class PersistenceDatabase {
  public:
    explicit PersistenceDatabase(const core::Logger* logger = nullptr,
                                 std::string DatabasePath = {});
    PersistenceDatabase(const PersistenceDatabase&) = delete;
    PersistenceDatabase& operator=(const PersistenceDatabase&) = delete;
    ~PersistenceDatabase();

    /// Open the database and ensure the schema is up to date.
    bool Initialize();

    /// Return a handle to the open database connection.
    QSqlDatabase Connection() const;

    /// Filesystem path of the database file.
    const std::string& DatabasePath() const noexcept;

    /// Platform-default path for the application database.
    static std::string DefaultDatabasePath(const core::Logger* logger = nullptr);

  private:
    void _LogInfo(const std::string& message) const;
    void _LogWarning(const std::string& message) const;
    void _LogError(const std::string& message) const;

    bool _Open();
    bool _EnsureSchema();
    bool _ExecuteStatement(const char* sql) const;
    bool _TableHasColumn(const char* tableName, const char* columnName) const;
    int _SchemaVersion() const;
    bool _SetSchemaVersion(int version) const;

    const core::Logger* m_logger{nullptr};
    std::string m_databasePath;
    std::string m_connectionName;
    mutable std::unique_ptr<QSqlDatabase> m_database;
    bool m_initialized{false};
};
}  // namespace xaimassist::persistence

#endif  // PERSISTENCEDATABASE_HPP
