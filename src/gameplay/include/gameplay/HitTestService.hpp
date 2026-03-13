/**
 * @file HitTestService.hpp
 * @brief Abstract ray-cast hit detection interface.
 */

#ifndef HITTESTSERVICE_HPP
#define HITTESTSERVICE_HPP

#include <cstdint>
#include <optional>

#include "core/CoreEvents.hpp"

namespace xaimassist::gameplay {

/// Outcome of a successful hit-test ray cast.
struct HitTestResult {
    std::uint64_t TargetId{0};
};

/**
 * @class IHitTestService
 * @brief Interface used by TargetSystem to resolve shot-to-target hits.
 */
class IHitTestService {
  public:
    virtual ~IHitTestService() = default;

    /// Cast a ray from the camera centre; returns the hit target if any.
    virtual std::optional<HitTestResult>
    CastShotRay(const core::events::ShotFiredEvent& shotFiredEvent) const = 0;
};
}  // namespace xaimassist::gameplay

#endif  // HITTESTSERVICE_HPP
