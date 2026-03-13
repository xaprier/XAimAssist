/**
 * @file TrackingTargetsMode.hpp
 * @brief Continuous tracking practice against a single bouncing target.
 */

#ifndef TRACKINGTARGETSMODE_HPP
#define TRACKINGTARGETSMODE_HPP

#include <array>
#include <cstdint>
#include <random>

#include "gameplay/GameMode.hpp"

namespace xaimassist::gameplay {

/**
 * @class TrackingTargetsMode
 * @brief Spawns a moving target that bounces off virtual bounds;
 *        periodic auto-hit checks reward continuous aim tracking.
 */
class TrackingTargetsMode final : public GameMode {
  public:
    TrackingTargetsMode(core::EventBus& eventBus, core::Logger& logger,
                        ISceneCommandSink& sceneCommandSink,
                        TargetSystem& targetSystem,
                        IGameplaySettingsProvider& settingsProvider);

    /// Return the static metadata descriptor for TrackingTargets.
    static GameModeMetadata MetadataDefinition();

    /// @copydoc GameMode::OnStart()
    void OnStart() override;

    /// @copydoc GameMode::OnUpdate()
    void OnUpdate(double dtSeconds) override;

    /// @copydoc GameMode::OnStop()
    void OnStop() override;

  private:
    bool _SpawnMovingTargetAtRandomSlot();
    std::array<double, 2> _RandomSpawnOffset();
    std::array<double, 2> _RandomUnitPlanarDirection();
    void _UpdateMovementDirectionAfterBounce(bool hitYBound, bool hitZBound);

    std::uint64_t m_activeTargetId{0};
    std::mt19937 m_randomEngine;
    std::array<double, 3> m_activeTargetPosition{0.0, 0.0, 0.0};
    std::array<double, 2> m_activeMovementDirection{0.0, 1.0};
    double m_activeMovementSpeed{2.5};
    double m_autoHitCheckAccumulatorSeconds{0.0};
};
}  // namespace xaimassist::gameplay

#endif  // TRACKINGTARGETSMODE_HPP
