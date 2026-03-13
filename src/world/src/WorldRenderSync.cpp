/// @file WorldRenderSync.cpp
#include "world/WorldRenderSync.hpp"

#include <unordered_set>

namespace xaimassist::world {
WorldRenderSync::WorldRenderSync(World& world,
                                 IRenderSceneBackend& renderBackend)
    : m_world(world), m_renderBackend(renderBackend) {}

void WorldRenderSync::Sync() {
    const std::uint64_t currentTopologyRevision =
        m_world.RenderTopologyRevision();
    const bool topologyChanged =
        currentTopologyRevision != m_lastTopologyRevision;
    const bool hasDirtyEntities = m_world.RenderDirtyCount() > 0;

    // Fast-path: skip sync when nothing changed
    if (!topologyChanged && !hasDirtyEntities) {
        return;
    }

    auto syncEntity = [&](EntityId entityId) {
        const auto* transformComponent = m_world.Transform(entityId);
        const auto* renderComponent = m_world.Render(entityId);
        if (transformComponent == nullptr || renderComponent == nullptr ||
            !renderComponent->visible) {
            m_world.ClearRenderDirty(entityId);
            return;
        }

        auto bindingIterator = m_renderBindings.find(entityId);
        if (bindingIterator == m_renderBindings.end()) {
            IRenderSceneBackend::RenderObjectId renderObjectId = 0;
            switch (renderComponent->primitiveType) {
                case RenderPrimitiveType::Sphere:
                    renderObjectId =
                        m_renderBackend.CreateSphere(*transformComponent, *renderComponent);
                    break;
            }

            if (renderObjectId != 0) {
                m_renderBindings.emplace(entityId, renderObjectId);
                m_world.ClearRenderDirty(entityId);
            }

            return;
        }

        if (!m_world.IsRenderDirty(entityId)) {
            return;
        }

        bool updated = false;
        switch (renderComponent->primitiveType) {
            case RenderPrimitiveType::Sphere:
                updated = m_renderBackend.UpdateSphere(
                    bindingIterator->second, *transformComponent, *renderComponent);
                break;
        }

        if (updated) {
            m_world.ClearRenderDirty(entityId);
        }
    };

    if (topologyChanged) {
        // Prune render bindings for destroyed entities
        const auto renderableEntities = m_world.EntitiesWithRenderable();
        const std::unordered_set<EntityId> activeEntities(
            renderableEntities.begin(), renderableEntities.end());

        for (auto iterator = m_renderBindings.begin();
             iterator != m_renderBindings.end();) {
            if (activeEntities.find(iterator->first) != activeEntities.end()) {
                ++iterator;
                continue;
            }

            m_renderBackend.DestroyObject(iterator->second);
            iterator = m_renderBindings.erase(iterator);
        }

        for (const EntityId entityId : renderableEntities) {
            syncEntity(entityId);
        }

        m_lastTopologyRevision = currentTopologyRevision;
        return;
    }

    const auto dirtyEntities = m_world.RenderDirtyEntities();
    for (const EntityId entityId : dirtyEntities) {
        syncEntity(entityId);
    }
}

void WorldRenderSync::Reset() {
    for (const auto& [_, objectId] : m_renderBindings) {
        m_renderBackend.DestroyObject(objectId);
    }

    m_renderBindings.clear();
    m_lastTopologyRevision = 0;
}
}  // namespace xaimassist::world
