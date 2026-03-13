/**
 * @file TargetSystem.hpp
 * @brief Manages live targets, hit resolution and per-frame updates.
 */

#ifndef TARGETSYSTEM_HPP
#define TARGETSYSTEM_HPP

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "gameplay/TargetFactory.hpp"

namespace xaimassist::core {
class Logger;
}

namespace xaimassist::gameplay {
class IHitTestService;
class ISceneCommandSink;
class Target;
struct TargetDefinition;

/**
 * @class TargetSystem
 * @brief Owns spawned targets, ticks their behaviour, and resolves
 *        shot-fired events into hit/miss outcomes via IHitTestService.
 */
class TargetSystem {
  public:
    using TargetId = std::uint64_t;

    TargetSystem(core::EventBus& eventBus, core::Logger& logger,
                 ISceneCommandSink& sceneCommandSink,
                 IHitTestService& hitTestService,
                 TargetFactory targetFactory = TargetFactory{});

    ~TargetSystem();

    /// Spawn a new target from a definition and return its id.
    TargetId SpawnTarget(std::uint64_t SessionId,
                         const TargetDefinition& Definition);

    /// Tick all live targets and process pending shot-fired events.
    void Update(double dtSeconds);

    /// Repaint all live targets with the given colour.
    void UpdateAllTargetColors(const std::array<double, 3>& color);

    /// Remove all targets belonging to a specific session.
    void ClearSessionTargets(std::uint64_t SessionId);

    /// Remove every target regardless of session.
    void ClearAllTargets();

    /// True if a target with the given id is still alive.
    bool HasTarget(TargetId targetId) const;

  private:
    void _SubscribeToEvents();
    void _UnsubscribeFromEvents();
    void _HandleCoreEvent(const core::events::CoreEvent& event);
    void _OnShotFired(const core::events::ShotFiredEvent& shotFiredEvent);

    core::EventBus& m_eventBus;
    core::Logger& m_logger;
    ISceneCommandSink& m_sceneCommandSink;
    IHitTestService& m_hitTestService;
    TargetFactory m_targetFactory;

    core::EventBus::SubscriptionId m_subscriptionId{0};
    std::unordered_map<TargetId, std::unique_ptr<Target>> m_targets;
};
}  // namespace xaimassist::gameplay

#endif  // TARGETSYSTEM_HPP
