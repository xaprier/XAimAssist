/**
 * @file GameplayWorldBridge.hpp
 * @brief Adapts ISceneCommandSink to the World entity-component store.
 */

#ifndef GAMEPLAYWORLDBRIDGE_HPP
#define GAMEPLAYWORLDBRIDGE_HPP

#include <unordered_map>

#include "gameplay/SceneCommandSink.hpp"
#include "world/World.hpp"

namespace xaimassist::app {

/**
 * @class GameplayWorldBridge
 * @brief Translates gameplay spawn/update/destroy commands into
 *        World entity mutations.
 */
class GameplayWorldBridge : public gameplay::ISceneCommandSink {
  public:
    explicit GameplayWorldBridge(world::World& world);

    /// Spawn a sphere target as a world entity.
    gameplay::SceneObjectId SpawnSphereTarget(const gameplay::SphereTargetSpawnRequest& request) override;

    /// Move an existing target entity to a new position.
    bool UpdateTargetPosition(gameplay::SceneObjectId objectId, const std::array<double, 3>& position) override;

    /// Change a target entity's colour.
    bool UpdateTargetColor(gameplay::SceneObjectId objectId, const std::array<double, 3>& color) override;

    /// Show or hide a target entity.
    bool UpdateTargetVisibility(gameplay::SceneObjectId objectId, bool visible) override;

    /// Destroy a target entity and its world components.
    bool DestroyTarget(gameplay::SceneObjectId objectId) override;

    /// Clear all target entities from the world.
    void ClearTargets() override;

  private:
    world::World& m_world;
    std::unordered_map<gameplay::SceneObjectId, world::EntityId> m_targetEntities;
};
}  // namespace xaimassist::app

#endif  // GAMEPLAYWORLDBRIDGE_HPP
