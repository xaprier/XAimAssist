/**
 * @file SceneCommandSink.hpp
 * @brief Interface for gameplay-issued scene mutations.
 *
 * Gameplay code spawns/updates/destroys targets through this
 * abstraction without touching engine or world internals.
 */

#ifndef SCENECOMMANDSINK_HPP
#define SCENECOMMANDSINK_HPP

#include <array>
#include <cstdint>

namespace xaimassist::gameplay {

using SceneObjectId = std::uint64_t;

/// Data needed to spawn a sphere target in the scene.
struct SphereTargetSpawnRequest {
    double radius{0.5};
    std::array<double, 3> position{0.0, 0.0, 0.0};
    std::array<double, 3> color{0.95, 0.35, 0.25};
    double opacity{1.0};
    bool collidable{true};
};

/**
 * @class ISceneCommandSink
 * @brief Abstract command interface for target scene manipulation.
 */
class ISceneCommandSink {
  public:
    virtual ~ISceneCommandSink() = default;

    /// Spawn a sphere target and return its scene object id.
    virtual SceneObjectId
    SpawnSphereTarget(const SphereTargetSpawnRequest& request) = 0;

    /// Move an existing target to a new position.
    virtual bool UpdateTargetPosition(SceneObjectId objectId,
                                      const std::array<double, 3>& position) = 0;

    /// Change a target's diffuse colour.
    virtual bool UpdateTargetColor(SceneObjectId objectId,
                                   const std::array<double, 3>& color) = 0;

    /// Show or hide a target without destroying it.
    virtual bool UpdateTargetVisibility(SceneObjectId objectId, bool visible) = 0;

    /// Remove a target from the scene permanently.
    virtual bool DestroyTarget(SceneObjectId objectId) = 0;

    /// Remove all targets from the scene.
    virtual void ClearTargets() = 0;
};
}  // namespace xaimassist::gameplay

#endif  // SCENECOMMANDSINK_HPP
