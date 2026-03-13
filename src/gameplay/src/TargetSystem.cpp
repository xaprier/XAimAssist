/// @file TargetSystem.cpp
#include "gameplay/TargetSystem.hpp"

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

#include "core/EventBus.hpp"
#include "core/Logger.hpp"
#include "gameplay/HitTestService.hpp"
#include "gameplay/SceneCommandSink.hpp"
#include "gameplay/TargetFactory.hpp"
#include "gameplay/targets/Target.hpp"

namespace {
double toReactionMs(std::chrono::steady_clock::time_point SpawnedAt,
                    std::chrono::steady_clock::time_point shotAt) {
    if (shotAt <= SpawnedAt) {
        return 0.0;
    }

    return std::chrono::duration<double, std::milli>(shotAt - SpawnedAt).count();
}
}  // namespace

namespace xaimassist::gameplay {
TargetSystem::TargetSystem(core::EventBus& eventBus, core::Logger& logger,
                           ISceneCommandSink& sceneCommandSink,
                           IHitTestService& hitTestService,
                           TargetFactory targetFactory)
    : m_eventBus(eventBus), m_logger(logger), m_sceneCommandSink(sceneCommandSink), m_hitTestService(hitTestService), m_targetFactory(std::move(targetFactory)) {
    _SubscribeToEvents();
}

TargetSystem::~TargetSystem() {
    ClearAllTargets();
    _UnsubscribeFromEvents();
}

TargetSystem::TargetId
TargetSystem::SpawnTarget(std::uint64_t SessionId,
                          const TargetDefinition& Definition) {
    if (SessionId == 0) {
        return 0;
    }

    SphereTargetSpawnRequest spawnRequest;
    spawnRequest.radius = Definition.radius;
    spawnRequest.position = Definition.initialPosition;
    spawnRequest.color = Definition.color;
    spawnRequest.collidable = Definition.collidable;

    const SceneObjectId sceneObjectId =
        m_sceneCommandSink.SpawnSphereTarget(spawnRequest);
    if (sceneObjectId == 0) {
        m_logger.Error(
            "gameplay",
            "Target spawn failed: scene object creation returned invalid id");
        return 0;
    }

    const auto SpawnedAt = std::chrono::steady_clock::now();
    const TargetId targetId = static_cast<TargetId>(sceneObjectId);

    auto target = m_targetFactory.CreateTarget(targetId, SessionId, sceneObjectId,
                                               Definition, SpawnedAt);
    if (!target) {
        m_sceneCommandSink.DestroyTarget(sceneObjectId);
        m_logger.Error(
            "gameplay",
            "Target spawn failed: no target implementation for Definition");
        return 0;
    }

    m_targets.emplace(targetId, std::move(target));

    m_eventBus.Publish(
        core::events::TargetSpawnedEvent{SessionId, targetId, SpawnedAt});

    return targetId;
}

void TargetSystem::Update(double dtSeconds) {
    const double safeDelta = std::max(0.0, dtSeconds);
    for (auto& [_, target] : m_targets) {
        if (target->State() != TargetState::Alive) {
            continue;
        }

        target->Update(safeDelta, m_sceneCommandSink);
    }
}

void TargetSystem::UpdateAllTargetColors(const std::array<double, 3>& color) {
    for (auto& [_, target] : m_targets) {
        if (target->State() != TargetState::Alive) {
            continue;
        }

        target->SetColor(m_sceneCommandSink, color);
    }
}

void TargetSystem::ClearSessionTargets(std::uint64_t SessionId) {
    if (SessionId == 0) {
        return;
    }

    std::vector<TargetId> targetsToDestroy;
    targetsToDestroy.reserve(m_targets.size());

    for (const auto& [targetId, target] : m_targets) {
        if (target->SessionId() == SessionId) {
            targetsToDestroy.push_back(targetId);
        }
    }

    const auto timestamp = std::chrono::steady_clock::now();
    for (const auto targetId : targetsToDestroy) {
        const auto iterator = m_targets.find(targetId);
        if (iterator == m_targets.end()) {
            continue;
        }

        m_sceneCommandSink.DestroyTarget(iterator->second->GetSceneObjectId());

        m_eventBus.Publish(core::events::TargetDestroyedEvent{SessionId, targetId,
                                                              false, timestamp});

        m_targets.erase(iterator);
    }
}

void TargetSystem::ClearAllTargets() {
    std::vector<TargetId> targetsToDestroy;
    targetsToDestroy.reserve(m_targets.size());

    for (const auto& [targetId, _] : m_targets) {
        targetsToDestroy.push_back(targetId);
    }

    const auto timestamp = std::chrono::steady_clock::now();
    for (const auto targetId : targetsToDestroy) {
        const auto iterator = m_targets.find(targetId);
        if (iterator == m_targets.end()) {
            continue;
        }

        const auto SessionId = iterator->second->SessionId();
        m_sceneCommandSink.DestroyTarget(iterator->second->GetSceneObjectId());

        m_eventBus.Publish(core::events::TargetDestroyedEvent{SessionId, targetId,
                                                              false, timestamp});

        m_targets.erase(iterator);
    }
}

bool TargetSystem::HasTarget(TargetId targetId) const {
    return m_targets.find(targetId) != m_targets.end();
}

void TargetSystem::_SubscribeToEvents() {
    if (m_subscriptionId != 0) {
        return;
    }

    m_subscriptionId =
        m_eventBus.Subscribe([this](const core::events::CoreEvent& event) {
            _HandleCoreEvent(event);
        });
}

void TargetSystem::_UnsubscribeFromEvents() {
    if (m_subscriptionId == 0) {
        return;
    }

    m_eventBus.Unsubscribe(m_subscriptionId);
    m_subscriptionId = 0;
}

void TargetSystem::_HandleCoreEvent(const core::events::CoreEvent& event) {
    if (const auto* shotFiredEvent =
            std::get_if<core::events::ShotFiredEvent>(&event)) {
        _OnShotFired(*shotFiredEvent);
    }
}

void TargetSystem::_OnShotFired(
    const core::events::ShotFiredEvent& shotFiredEvent) {
    const auto hitResult = m_hitTestService.CastShotRay(shotFiredEvent);
    if (!hitResult.has_value()) {
        m_eventBus.Publish(core::events::ShotMissEvent{shotFiredEvent.sessionId,
                                                       shotFiredEvent.timestamp});
        return;
    }

    const TargetId targetId = hitResult->TargetId;
    const auto iterator = m_targets.find(targetId);
    if (iterator == m_targets.end()) {
        m_eventBus.Publish(core::events::ShotMissEvent{shotFiredEvent.sessionId,
                                                       shotFiredEvent.timestamp});
        return;
    }

    auto& target = *iterator->second;
    if (target.SessionId() != shotFiredEvent.sessionId ||
        target.State() != TargetState::Alive) {
        m_eventBus.Publish(core::events::ShotMissEvent{shotFiredEvent.sessionId,
                                                       shotFiredEvent.timestamp});
        return;
    }

    const double reactionTimeMs =
        toReactionMs(target.SpawnedAt(), shotFiredEvent.timestamp);
    target.OnHit(m_sceneCommandSink);

    m_eventBus.Publish(core::events::ShotHitEvent{shotFiredEvent.sessionId,
                                                  targetId, reactionTimeMs,
                                                  shotFiredEvent.timestamp});

    if (target.State() == TargetState::Destroyed) {
        m_eventBus.Publish(core::events::TargetDestroyedEvent{
            shotFiredEvent.sessionId, targetId, true, shotFiredEvent.timestamp});
        m_targets.erase(iterator);
    }
}
}  // namespace xaimassist::gameplay
