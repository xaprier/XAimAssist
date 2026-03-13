/// @file GameplayWorldBridge.cpp
#include "app/GameplayWorldBridge.hpp"

namespace xaimassist::app {
GameplayWorldBridge::GameplayWorldBridge(world::World& world)
    : m_world(world) {}

gameplay::SceneObjectId GameplayWorldBridge::SpawnSphereTarget(
    const gameplay::SphereTargetSpawnRequest& request) {
    const world::EntityId entityId = m_world.CreateEntity();

    world::TransformComponent transformComponent;
    transformComponent.position = request.position;
    m_world.UpsertTransform(entityId, transformComponent);

    world::RenderComponent renderComponent;
    renderComponent.primitiveType = world::RenderPrimitiveType::Sphere;
    renderComponent.color = request.color;
    renderComponent.sphereRadius = request.radius;
    renderComponent.opacity = request.opacity;
    m_world.UpsertRender(entityId, renderComponent);

    if (request.collidable) {
        world::ColliderComponent colliderComponent;
        colliderComponent.shapeType = world::ColliderShapeType::Sphere;
        colliderComponent.sphereRadius = request.radius;
        m_world.UpsertCollider(entityId, colliderComponent);
    }

    const gameplay::SceneObjectId objectId =
        static_cast<gameplay::SceneObjectId>(entityId);
    m_targetEntities.emplace(objectId, entityId);
    return objectId;
}

bool GameplayWorldBridge::UpdateTargetPosition(
    gameplay::SceneObjectId objectId, const std::array<double, 3>& position) {
    auto iterator = m_targetEntities.find(objectId);
    if (iterator == m_targetEntities.end()) {
        return false;
    }

    const world::EntityId entityId = iterator->second;
    if (!m_world.IsAlive(entityId)) {
        m_targetEntities.erase(iterator);
        return false;
    }

    world::TransformComponent transformComponent;
    if (const auto* existingTransform = m_world.Transform(entityId)) {
        transformComponent = *existingTransform;
    }

    transformComponent.position = position;
    return m_world.UpsertTransform(entityId, transformComponent);
}

bool GameplayWorldBridge::UpdateTargetColor(
    gameplay::SceneObjectId objectId, const std::array<double, 3>& color) {
    auto iterator = m_targetEntities.find(objectId);
    if (iterator == m_targetEntities.end()) {
        return false;
    }

    const world::EntityId entityId = iterator->second;
    if (!m_world.IsAlive(entityId)) {
        m_targetEntities.erase(iterator);
        return false;
    }

    world::RenderComponent renderComponent;
    if (const auto* existingRender = m_world.Render(entityId)) {
        renderComponent = *existingRender;
    }

    renderComponent.color = color;
    return m_world.UpsertRender(entityId, renderComponent);
}

bool GameplayWorldBridge::UpdateTargetVisibility(
    gameplay::SceneObjectId objectId, bool visible) {
    auto iterator = m_targetEntities.find(objectId);
    if (iterator == m_targetEntities.end()) {
        return false;
    }

    const world::EntityId entityId = iterator->second;
    if (!m_world.IsAlive(entityId)) {
        m_targetEntities.erase(iterator);
        return false;
    }

    const auto* existingRender = m_world.Render(entityId);
    if (existingRender == nullptr) {
        return false;
    }

    world::RenderComponent renderComponent = *existingRender;
    renderComponent.visible = visible;
    return m_world.UpsertRender(entityId, renderComponent);
}

bool GameplayWorldBridge::DestroyTarget(gameplay::SceneObjectId objectId) {
    auto iterator = m_targetEntities.find(objectId);
    if (iterator == m_targetEntities.end()) {
        return false;
    }

    const world::EntityId entityId = iterator->second;
    m_targetEntities.erase(iterator);
    return m_world.DestroyEntity(entityId);
}

void GameplayWorldBridge::ClearTargets() {
    for (const auto& [_, entityId] : m_targetEntities) {
        m_world.DestroyEntity(entityId);
    }

    m_targetEntities.clear();
}
}  // namespace xaimassist::app
