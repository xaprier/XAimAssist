/**
 * @file World.hpp
 * @brief Lightweight entity-component store for the game world.
 */

#ifndef WORLD_HPP
#define WORLD_HPP

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "world/Components.hpp"

namespace xaimassist::world {

using EntityId = std::uint64_t;

/**
 * @class World
 * @brief Flat entity-component container with change-tracking for render sync.
 *
 * Entities carry optional Transform, Render, and Collider components.
 * The dirty set and topology revision let WorldRenderSync push only
 * the entities that actually changed.
 */
class World {
  public:
    /// Allocate a fresh entity and return its id.
    EntityId CreateEntity();

    /// Remove an entity and all its components.
    bool DestroyEntity(EntityId entityId);

    /// Destroy every entity in the world.
    void Clear();

    /// True if the entity has not been destroyed.
    bool IsAlive(EntityId entityId) const;

    /// Number of live entities.
    std::size_t EntityCount() const noexcept;

    /// Snapshot of all live entity ids.
    std::vector<EntityId> Entities() const;

    /// Insert or replace a component on an existing entity.
    ///@{
    bool UpsertTransform(EntityId entityId, const TransformComponent& Transform);
    bool UpsertRender(EntityId entityId, const RenderComponent& Render);
    bool UpsertCollider(EntityId entityId, const ColliderComponent& Collider);
    ///@}

    /// Remove the transform component from an entity.
    bool RemoveTransform(EntityId entityId);

    /// Remove the render component from an entity.
    bool RemoveRender(EntityId entityId);

    /// Remove the collider component from an entity.
    bool RemoveCollider(EntityId entityId);

    /// True if the entity has a transform component.
    bool HasTransform(EntityId entityId) const;

    /// True if the entity has a render component.
    bool HasRender(EntityId entityId) const;

    /// True if the entity has a collider component.
    bool HasCollider(EntityId entityId) const;

    /// Returns nullptr when the entity or component does not exist.
    ///@{
    const TransformComponent* Transform(EntityId entityId) const;
    const RenderComponent* Render(EntityId entityId) const;
    const ColliderComponent* Collider(EntityId entityId) const;
    ///@}

    /// All entities that have both Transform and Render components.
    std::vector<EntityId> EntitiesWithRenderable() const;

    /// @name Render dirty tracking
    ///@{
    /// Flag an entity as needing a render-backend update.
    void MarkRenderDirty(EntityId entityId);

    /// True if the entity is in the dirty set.
    bool IsRenderDirty(EntityId entityId) const;

    /// Remove an entity from the dirty set.
    void ClearRenderDirty(EntityId entityId);

    /// Number of entities currently flagged dirty.
    std::size_t RenderDirtyCount() const noexcept;

    /// Snapshot of all dirty entity ids.
    std::vector<EntityId> RenderDirtyEntities() const;

    /// Monotonic counter bumped on structural changes (create/destroy).
    std::uint64_t RenderTopologyRevision() const noexcept;
    ///@}

  private:
    bool _ValidateEntity(EntityId entityId) const;
    bool _IsRenderable(EntityId entityId) const;

    EntityId m_nextEntityId{1};
    std::unordered_set<EntityId> m_entities;
    std::unordered_map<EntityId, TransformComponent> m_transformComponents;
    std::unordered_map<EntityId, RenderComponent> m_renderComponents;
    std::unordered_map<EntityId, ColliderComponent> m_colliderComponents;
    std::unordered_set<EntityId> m_renderDirtyEntities;
    std::uint64_t m_renderTopologyRevision{1};
};
}  // namespace xaimassist::world

#endif  // WORLD_HPP
