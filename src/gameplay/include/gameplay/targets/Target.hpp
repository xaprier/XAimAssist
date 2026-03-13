/**
 * @file Target.hpp
 * @brief Target base class and concrete subclasses (Static, Moving, etc.).
 */

#ifndef TARGET_HPP
#define TARGET_HPP

#include <array>
#include <chrono>
#include <cstdint>

#include "gameplay/SceneCommandSink.hpp"

namespace xaimassist::gameplay {

/// Classification of target behaviour.
enum class TargetType { Static,
                        Moving,
                        Strafing,
                        Blinking };

/// Alive/destroyed lifecycle state of a target.
enum class TargetState { Alive,
                         Destroyed };

/// Blueprint used by TargetFactory to configure a new target instance.
struct TargetDefinition {
    TargetType type{TargetType::Static};
    double radius{0.5};
    std::array<double, 3> initialPosition{0.0, 0.0, 0.0};
    std::array<double, 3> color{0.95, 0.35, 0.25};
    bool collidable{true};
    bool destroyOnHit{true};

    std::array<double, 3> movementVelocity{0.8, 0.0, 0.0};
    double movementExtent{0.8};

    double blinkVisibleSeconds{0.65};
    double blinkHiddenSeconds{0.35};
};

/**
 * @class Target
 * @brief Abstract base for all in-scene targets.
 *
 * Holds identity, scene binding, and common position/visibility helpers.
 * Concrete subtypes implement Update() for movement behaviour.
 */
class Target {
  public:
    Target(std::uint64_t TargetId, std::uint64_t SessionId,
           SceneObjectId sceneObjId, TargetDefinition definition,
           std::chrono::steady_clock::time_point SpawnedAt);
    virtual ~Target() = default;

    /// Per-frame behaviour update (movement, blinking, etc.).
    virtual void Update(double dtSeconds,
                        ISceneCommandSink& sceneCommandSink) = 0;

    /// Handle being hit; returns true if the target was destroyed.
    virtual bool OnHit(ISceneCommandSink& sceneCommandSink);

    /// Change the target's colour via the scene command sink.
    bool SetColor(ISceneCommandSink& sceneCommandSink,
                  const std::array<double, 3>& color);

    /// Unique target identifier for the session.
    std::uint64_t TargetId() const noexcept;

    /// Session this target belongs to.
    std::uint64_t SessionId() const noexcept;

    /// Corresponding scene object id used by ISceneCommandSink.
    SceneObjectId GetSceneObjectId() const noexcept;

    /// Current alive/destroyed state.
    TargetState State() const noexcept;

    /// Timestamp when the target was spawned.
    std::chrono::steady_clock::time_point SpawnedAt() const noexcept;

  protected:
    /// Access the original definition blueprint.
    const TargetDefinition& Definition() const noexcept;

    /// Current world position.
    const std::array<double, 3>& CurrentPosition() const noexcept;

    /// Move the target to a new position through the scene sink.
    bool ApplyPosition(ISceneCommandSink& sceneCommandSink,
                       const std::array<double, 3>& newPosition);

    /// Show or hide the target without destroying it.
    bool ApplyVisibility(ISceneCommandSink& sceneCommandSink, bool visible);

  private:
    std::uint64_t m_targetId{0};
    std::uint64_t m_sessionId{0};
    SceneObjectId m_sceneObjectId{0};
    TargetDefinition m_definition;
    TargetState m_state{TargetState::Alive};
    std::chrono::steady_clock::time_point m_spawnedAt;
    std::array<double, 3> m_currentPosition{0.0, 0.0, 0.0};
};

/// Stationary target — no per-frame movement.
class StaticTarget final : public Target {
  public:
    using Target::Target;

    void Update(double dtSeconds, ISceneCommandSink& sceneCommandSink) override;
};

/// Target that oscillates linearly along a single axis.
class MovingTarget final : public Target {
  public:
    MovingTarget(std::uint64_t TargetId, std::uint64_t SessionId,
                 SceneObjectId sceneObjId, TargetDefinition definition,
                 std::chrono::steady_clock::time_point SpawnedAt);

    void Update(double dtSeconds, ISceneCommandSink& sceneCommandSink) override;

  private:
    std::array<double, 3> m_anchorPosition{0.0, 0.0, 0.0};
    std::array<double, 3> m_axis{1.0, 0.0, 0.0};
    double m_speed{0.8};
    double m_extent{0.8};
    double m_offset{0.0};
    double m_direction{1.0};
};

/// Target that moves in a sinusoidal strafe pattern.
class StrafingTarget final : public Target {
  public:
    StrafingTarget(std::uint64_t TargetId, std::uint64_t SessionId,
                   SceneObjectId sceneObjId, TargetDefinition definition,
                   std::chrono::steady_clock::time_point SpawnedAt);

    void Update(double dtSeconds, ISceneCommandSink& sceneCommandSink) override;

  private:
    std::array<double, 3> m_anchorPosition{0.0, 0.0, 0.0};
    double m_elapsedSeconds{0.0};
    double m_angularSpeed{2.0};
    double m_extent{0.8};
};

/// Target that toggles visibility on a timer.
class BlinkingTarget final : public Target {
  public:
    BlinkingTarget(std::uint64_t TargetId, std::uint64_t SessionId,
                   SceneObjectId sceneObjId, TargetDefinition definition,
                   std::chrono::steady_clock::time_point SpawnedAt);

    void Update(double dtSeconds, ISceneCommandSink& sceneCommandSink) override;

  private:
    double m_phaseSeconds{0.0};
    double m_visibleSeconds{0.65};
    double m_hiddenSeconds{0.35};
    bool m_visible{true};
};
}  // namespace xaimassist::gameplay

#endif  // TARGET_HPP
