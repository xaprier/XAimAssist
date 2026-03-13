/**
 * @file WorldRaycastHitTest.hpp
 * @brief IHitTestService using world colliders and engine camera ray.
 */

#ifndef WORLDRAYCASTHITTEST_HPP
#define WORLDRAYCASTHITTEST_HPP

#include "gameplay/HitTestService.hpp"

namespace xaimassist::engine {
class Engine;
}

namespace xaimassist::world {
class World;
}

namespace xaimassist::app {

/**
 * @class WorldRaycastHitTest
 * @brief Casts a ray from the camera centre through world colliders
 *        to determine which target (if any) was hit.
 */
class WorldRaycastHitTest final : public gameplay::IHitTestService {
  public:
    WorldRaycastHitTest(const world::World& world, const engine::Engine& engine);

    /// Cast a ray from the camera centre and test against world colliders.
    std::optional<gameplay::HitTestResult> CastShotRay(
        const core::events::ShotFiredEvent& shotFiredEvent) const override;

  private:
    const world::World& m_world;
    const engine::Engine& m_engine;
};
}  // namespace xaimassist::app

#endif  // WORLDRAYCASTHITTEST_HPP
