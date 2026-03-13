/**
 * @file NextShotMode.hpp
 * @brief Target-chaining flick mode with a preview.
 */

#ifndef NEXTSHOTMODE_HPP
#define NEXTSHOTMODE_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <random>

#include "gameplay/GameMode.hpp"

namespace xaimassist::gameplay {

/**
 * @class NextShotMode
 * @brief Shows one active target plus a semi-transparent preview of
 *        the next; hitting the active promotes the preview.
 */
class NextShotMode final : public GameMode {
  public:
    NextShotMode(core::EventBus& eventBus, core::Logger& logger,
                 ISceneCommandSink& sceneCommandSink, TargetSystem& targetSystem,
                 IGameplaySettingsProvider& settingsProvider);

    /// Return the static metadata descriptor for NextShot.
    static GameModeMetadata MetadataDefinition();

    /// @copydoc GameMode::OnStart()
    void OnStart() override;

    /// @copydoc GameMode::OnUpdate()
    void OnUpdate(double dtSeconds) override;

    /// @copydoc GameMode::OnStop()
    void OnStop() override;

  private:
    bool _SpawnActiveTargetAtOffset(const std::array<double, 2>& spawnOffset);
    bool _SpawnPreviewTargetAtOffset(const std::array<double, 2>& spawnOffset);
    std::array<double, 2>
    _RandomSpawnOffset(double minimumDistance,
                       const std::optional<std::array<double, 2>>& avoidOffset);
    double _MinimumSpawnOffsetDistance() const;
    static double _PlanarDistance(const std::array<double, 2>& left,
                                  const std::array<double, 2>& right);

    std::uint64_t m_activeTargetId{0};
    SceneObjectId m_previewTargetObjectId{0};
    std::uint64_t m_shotHitSubscriptionId{0};
    std::mt19937 m_randomEngine;
    std::optional<std::array<double, 2>> m_activeSpawnOffset;
    std::optional<std::array<double, 2>> m_previewSpawnOffset;
};
}  // namespace xaimassist::gameplay

#endif  // NEXTSHOTMODE_HPP
