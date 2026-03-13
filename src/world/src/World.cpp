/// @file World.cpp
#include "world/World.hpp"

namespace xaimassist::world {
EntityId World::CreateEntity() {
    const EntityId entityId = m_nextEntityId++;
    m_entities.insert(entityId);
    return entityId;
}

bool World::DestroyEntity(EntityId entityId) {
    if (!_ValidateEntity(entityId)) {
        return false;
    }

    const bool wasRenderable = _IsRenderable(entityId);

    m_entities.erase(entityId);
    m_transformComponents.erase(entityId);
    m_renderComponents.erase(entityId);
    m_colliderComponents.erase(entityId);
    m_renderDirtyEntities.erase(entityId);

    if (wasRenderable) {
        ++m_renderTopologyRevision;
    }

    return true;
}

void World::Clear() {
    bool hadRenderableEntities = false;
    for (const auto& [entityId, renderComponent] : m_renderComponents) {
        if (!renderComponent.visible) {
            continue;
        }

        if (m_transformComponents.find(entityId) != m_transformComponents.end()) {
            hadRenderableEntities = true;
            break;
        }
    }

    m_entities.clear();
    m_transformComponents.clear();
    m_renderComponents.clear();
    m_colliderComponents.clear();
    m_renderDirtyEntities.clear();

    if (hadRenderableEntities) {
        ++m_renderTopologyRevision;
    }
}

bool World::IsAlive(EntityId entityId) const {
    return _ValidateEntity(entityId);
}

std::size_t World::EntityCount() const noexcept { return m_entities.size(); }

std::vector<EntityId> World::Entities() const {
    return std::vector<EntityId>(m_entities.begin(), m_entities.end());
}

bool World::UpsertTransform(EntityId entityId,
                            const TransformComponent& Transform) {
    if (!_ValidateEntity(entityId)) {
        return false;
    }

    const bool wasRenderable = _IsRenderable(entityId);

    m_transformComponents[entityId] = Transform;

    if (wasRenderable != _IsRenderable(entityId)) {
        ++m_renderTopologyRevision;
    }

    if (HasRender(entityId)) {
        MarkRenderDirty(entityId);
    }
    return true;
}

bool World::UpsertRender(EntityId entityId, const RenderComponent& Render) {
    if (!_ValidateEntity(entityId)) {
        return false;
    }

    const bool wasRenderable = _IsRenderable(entityId);

    m_renderComponents[entityId] = Render;

    if (wasRenderable != _IsRenderable(entityId)) {
        ++m_renderTopologyRevision;
    }

    MarkRenderDirty(entityId);
    return true;
}

bool World::UpsertCollider(EntityId entityId,
                           const ColliderComponent& Collider) {
    if (!_ValidateEntity(entityId)) {
        return false;
    }

    m_colliderComponents[entityId] = Collider;
    return true;
}

bool World::RemoveTransform(EntityId entityId) {
    if (!_ValidateEntity(entityId)) {
        return false;
    }

    const bool wasRenderable = _IsRenderable(entityId);

    const auto erased = m_transformComponents.erase(entityId);
    if (erased > 0) {
        if (wasRenderable) {
            ++m_renderTopologyRevision;
        }

        MarkRenderDirty(entityId);
    }
    return erased > 0;
}

bool World::RemoveRender(EntityId entityId) {
    if (!_ValidateEntity(entityId)) {
        return false;
    }

    const bool wasRenderable = _IsRenderable(entityId);

    const auto erased = m_renderComponents.erase(entityId);
    if (erased > 0) {
        if (wasRenderable) {
            ++m_renderTopologyRevision;
        }

        MarkRenderDirty(entityId);
    }
    return erased > 0;
}

bool World::RemoveCollider(EntityId entityId) {
    if (!_ValidateEntity(entityId)) {
        return false;
    }

    return m_colliderComponents.erase(entityId) > 0;
}

bool World::HasTransform(EntityId entityId) const {
    return m_transformComponents.find(entityId) != m_transformComponents.end();
}

bool World::HasRender(EntityId entityId) const {
    return m_renderComponents.find(entityId) != m_renderComponents.end();
}

bool World::HasCollider(EntityId entityId) const {
    return m_colliderComponents.find(entityId) != m_colliderComponents.end();
}

const TransformComponent* World::Transform(EntityId entityId) const {
    const auto iterator = m_transformComponents.find(entityId);
    if (iterator == m_transformComponents.end()) {
        return nullptr;
    }

    return &iterator->second;
}

const RenderComponent* World::Render(EntityId entityId) const {
    const auto iterator = m_renderComponents.find(entityId);
    if (iterator == m_renderComponents.end()) {
        return nullptr;
    }

    return &iterator->second;
}

const ColliderComponent* World::Collider(EntityId entityId) const {
    const auto iterator = m_colliderComponents.find(entityId);
    if (iterator == m_colliderComponents.end()) {
        return nullptr;
    }

    return &iterator->second;
}

std::vector<EntityId> World::EntitiesWithRenderable() const {
    std::vector<EntityId> result;
    result.reserve(m_renderComponents.size());

    for (const auto& [entityId, renderComponent] : m_renderComponents) {
        if (!renderComponent.visible) {
            continue;
        }

        if (!HasTransform(entityId)) {
            continue;
        }

        result.push_back(entityId);
    }

    return result;
}

void World::MarkRenderDirty(EntityId entityId) {
    if (!_ValidateEntity(entityId)) {
        return;
    }

    m_renderDirtyEntities.insert(entityId);
}

bool World::IsRenderDirty(EntityId entityId) const {
    return m_renderDirtyEntities.find(entityId) != m_renderDirtyEntities.end();
}

void World::ClearRenderDirty(EntityId entityId) {
    m_renderDirtyEntities.erase(entityId);
}

std::size_t World::RenderDirtyCount() const noexcept {
    return m_renderDirtyEntities.size();
}

std::vector<EntityId> World::RenderDirtyEntities() const {
    return std::vector<EntityId>(m_renderDirtyEntities.begin(),
                                 m_renderDirtyEntities.end());
}

std::uint64_t World::RenderTopologyRevision() const noexcept {
    return m_renderTopologyRevision;
}

bool World::_ValidateEntity(EntityId entityId) const {
    return m_entities.find(entityId) != m_entities.end();
}

bool World::_IsRenderable(EntityId entityId) const {
    if (!_ValidateEntity(entityId)) {
        return false;
    }

    const auto renderIterator = m_renderComponents.find(entityId);
    if (renderIterator == m_renderComponents.end() ||
        !renderIterator->second.visible) {
        return false;
    }

    return m_transformComponents.find(entityId) != m_transformComponents.end();
}
}  // namespace xaimassist::world
