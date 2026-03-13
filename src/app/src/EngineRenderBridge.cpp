/// @file EngineRenderBridge.cpp
#include "app/EngineRenderBridge.hpp"

#include <cmath>

#include "engine/Engine.hpp"

namespace xaimassist::app {
EngineRenderBridge::EngineRenderBridge(engine::Engine& engine)
    : m_engine(engine) {}

EngineRenderBridge::RenderObjectId
EngineRenderBridge::CreateSphere(const world::TransformComponent& transform,
                                 const world::RenderComponent& render) {
    engine::Engine::SphereSpawnRequest request;
    request.radius = render.sphereRadius;
    request.position = transform.position;
    request.color = render.color;
    request.opacity = render.opacity;
    const RenderObjectId objectId = m_engine.SpawnSphere(request);
    if (objectId != 0) {
        m_renderStateCache[objectId] =
            RenderStateCache{transform.position, render.color, render.opacity};
    }

    return objectId;
}

bool EngineRenderBridge::UpdateSphere(
    RenderObjectId objectId, const world::TransformComponent& transform,
    const world::RenderComponent& render) {
    auto cacheIterator = m_renderStateCache.find(objectId);
    if (cacheIterator == m_renderStateCache.end()) {
        m_renderStateCache[objectId] =
            RenderStateCache{transform.position, render.color, render.opacity};
        cacheIterator = m_renderStateCache.find(objectId);
    }

    RenderStateCache& cachedState = cacheIterator->second;

    // Diff against cached state to avoid redundant GPU calls
    const bool positionChanged = cachedState.position != transform.position;
    const bool colorChanged = cachedState.color != render.color;
    const bool opacityChanged =
        std::abs(cachedState.opacity - render.opacity) > 0.0001;

    if (!positionChanged && !colorChanged && !opacityChanged) {
        return true;
    }

    bool updated = true;
    if (positionChanged) {
        updated =
            m_engine.SetObjectPosition(objectId, transform.position) && updated;
    }

    if (colorChanged) {
        updated = m_engine.SetObjectColor(objectId, render.color) && updated;
    }

    if (opacityChanged) {
        updated = m_engine.SetObjectOpacity(objectId, render.opacity) && updated;
    }

    if (!updated) {
        return false;
    }

    cachedState.position = transform.position;
    cachedState.color = render.color;
    cachedState.opacity = render.opacity;
    return true;
}

bool EngineRenderBridge::DestroyObject(RenderObjectId objectId) {
    const bool destroyed = m_engine.DespawnObject(objectId);
    m_renderStateCache.erase(objectId);
    return destroyed;
}
}  // namespace xaimassist::app
