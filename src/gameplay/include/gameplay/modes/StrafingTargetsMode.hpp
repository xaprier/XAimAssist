/**
 * @file StrafingTargetsMode.hpp
 * @brief Two-phase mode: click static target, then track its moving variant.
 */

#ifndef STRAFINGTARGETSMODE_HPP
#define STRAFINGTARGETSMODE_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <random>

#include "gameplay/GameMode.hpp"

namespace xaimassist::gameplay {

/**
 * @class StrafingTargetsMode
 * @brief First spawns a static activation target, then converts it
 *        to a strafing target the player must track and hit.
 */
class StrafingTargetsMode final : public GameMode {
  public:
    StrafingTargetsMode(core::EventBus& eventBus, core::Logger& logger,
                        ISceneCommandSink& sceneCommandSink,
                        TargetSystem& targetSystem,
                        IGameplaySettingsProvider& settingsProvider);

    /// Return the static metadata descriptor for StrafingTargets.
    static GameModeMetadata MetadataDefinition();

    /// @copydoc GameMode::OnStart()
    void OnStart() override;

    /// @copydoc GameMode::OnUpdate()
    void OnUpdate(double dtSeconds) override;

    /// @copydoc GameMode::OnStop()
    void OnStop() override;

  private:
    enum class TargetStage {
        StaticAwaitingActivation,
        MovingAwaitingHit,
    };

    void _SpawnStaticTargetAtRandomSlot();
    bool _SpawnMovingTargetAtOffset(const std::array<double, 2>& spawnOffset,
                                    double speed);
    std::array<double, 2> _RandomSpawnOffset(double minimumDistance);
    std::array<double, 2> _RandomUnitPlanarDirection();
    static double _PlanarDistance(const std::array<double, 2>& left,
                                  const std::array<double, 2>& right);

    std::uint64_t m_activeTargetId{0};
    std::uint64_t m_shotHitSubscriptionId{0};
    std::mt19937 m_randomEngine;
    std::optional<std::array<double, 2>> m_previousStaticSpawnOffset;
    std::optional<std::array<double, 2>> m_activeTargetSpawnOffset;
    std::array<double, 3> m_activeTargetPosition{0.0, 0.0, 0.0};
    std::array<double, 2> m_activeMovementDirection{0.0, 1.0};
    double m_activeMovementSpeed{1.0};
    double m_movingElapsedSeconds{0.0};
    TargetStage m_targetStage{TargetStage::StaticAwaitingActivation};
};
}  // namespace xaimassist::gameplay

#endif  // STRAFINGTARGETSMODE_HPP
