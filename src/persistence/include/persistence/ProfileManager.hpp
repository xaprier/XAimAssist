/**
 * @file ProfileManager.hpp
 * @brief CRUD operations for user profiles stored in the database.
 */

#ifndef PROFILEMANAGER_HPP
#define PROFILEMANAGER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace xaimassist::core {
class Logger;
}

namespace xaimassist::persistence {
class PersistenceDatabase;

/// Flat row representation of a profile record.
struct ProfileRecord {
    std::int64_t id{0};
    std::string name;
    std::string createdAtIso;
    std::string updatedAtIso;
    bool active{false};
};

/**
 * @class ProfileManager
 * @brief Manages user profiles (create, list, activate) via SQL.
 */
class ProfileManager {
  public:
    ProfileManager(PersistenceDatabase& database, core::Logger& logger);

    /// Create the default profile if none exists.
    bool EnsureDefaultProfile();

    /// Create a new profile with the given name; returns its id (-1 on failure).
    std::int64_t CreateProfile(const std::string& profileName);

    /// Mark a profile as the active one.
    bool SetActiveProfile(std::int64_t profileId);

    /// Id of the currently active profile.
    std::int64_t ActiveProfileId() const;

    /// Full record of the currently active profile, if any.
    std::optional<ProfileRecord> ActiveProfile() const;

    /// All stored profiles.
    std::vector<ProfileRecord> Profiles() const;

  private:
    static std::string _NowIsoUtc();

    PersistenceDatabase& m_database;
    core::Logger& m_logger;
};
}  // namespace xaimassist::persistence

#endif  // PROFILEMANAGER_HPP
