/**
 * @file StaticSphereMode.hpp
 * @brief Single static-target flick training.
 */

#ifndef STATICSPHEREMODE_HPP
#define STATICSPHEREMODE_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <random>

#include "gameplay/GameMode.hpp"

namespace xaimassist::gameplay {

/**
 * @class StaticSphereMode
 * @brief Spawns one stationary target; hitting it spawns a new one
 *        at a random offset from the camera centre.
 */
class StaticSphereMode final : public GameMode {
  public:
    StaticSphereMode(core::EventBus& eventBus, core::Logger& logger,
                     ISceneCommandSink& sceneCommandSink,
                     TargetSystem& targetSystem,
                     IGameplaySettingsProvider& settingsProvider);

    /// Return the static metadata descriptor for StaticSphere.
    static GameModeMetadata MetadataDefinition();

    /// @copydoc GameMode::OnStart()
    void OnStart() override;

    /// @copydoc GameMode::OnUpdate()
    void OnUpdate(double dtSeconds) override;

    /// @copydoc GameMode::OnStop()
    void OnStop() override;

  private:
    void _SpawnTargetAtCurrentSlot();
    std::array<double, 2> _RandomSpawnOffset(double minimumDistance);
    static double _PlanarDistance(const std::array<double, 2>& left,
                                  const std::array<double, 2>& right);

    std::uint64_t m_activeTargetId{0};
    std::uint64_t m_shotHitSubscriptionId{0};
    std::mt19937 m_randomEngine;
    std::optional<std::array<double, 2>> m_previousSpawnOffset;
};
}  // namespace xaimassist::gameplay

#endif  // STATICSPHEREMODE_HPP
