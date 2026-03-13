/**
 * @file SessionStats.hpp
 * @brief Snapshot struct for real-time session metrics.
 */

#ifndef SESSIONSTATS_HPP
#define SESSIONSTATS_HPP

#include <cstdint>
#include <string>

namespace xaimassist::stats {

/// Point-in-time copy of accumulated session metrics.
struct SessionStatsSnapshot {
    std::uint64_t SessionId{0};
    std::string modeId;

    bool active{false};
    double ElapsedSeconds{0.0};

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
}  // namespace xaimassist::stats

#endif  // SESSIONSTATS_HPP
