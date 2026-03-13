/**
 * @file GridShotMode.hpp
 * @brief Grid-based flick training mode.
 */

#ifndef GRIDSHOTMODE_HPP
#define GRIDSHOTMODE_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <unordered_map>

#include "gameplay/GameMode.hpp"

namespace xaimassist::gameplay {

/**
 * @class GridShotMode
 * @brief Spawns targets on a configurable NxM grid; hitting one
 *        despawns it and spawns a replacement at a random free cell.
 */
class GridShotMode final : public GameMode {
  public:
    GridShotMode(core::EventBus& eventBus, core::Logger& logger,
                 ISceneCommandSink& sceneCommandSink, TargetSystem& targetSystem,
                 IGameplaySettingsProvider& settingsProvider);

    /// Return the static metadata descriptor for GridShot.
    static GameModeMetadata MetadataDefinition();

    /// Apply grid-specific settings (row/column/active-target counts).
    void ApplyModeSettings(const std::unordered_map<std::string, double>& requestedSettings) override;

    /// @copydoc GameMode::OnStart()
    void OnStart() override;

    /// @copydoc GameMode::OnUpdate()
    void OnUpdate(double dtSeconds) override;

    /// @copydoc GameMode::OnStop()
    void OnStop() override;

  private:
    void _SpawnTargetAtRandomAvailableCell(
        std::optional<std::size_t> excludedCellIndex = std::nullopt);
    std::optional<std::size_t>
    _RandomAvailableCellIndex(std::optional<std::size_t> excludedCellIndex);
    std::array<double, 3> _WorldPositionForCell(std::size_t cellIndex) const;
    bool _IsCellOccupied(std::size_t cellIndex) const;

    std::uint64_t m_shotHitSubscriptionId{0};
    std::mt19937 m_randomEngine;
    std::unordered_map<std::uint64_t, std::size_t> m_activeTargetCellById;
    std::size_t m_gridRowCount{5};
    std::size_t m_gridColumnCount{5};
    std::size_t m_activeTargetCount{3};
};
}  // namespace xaimassist::gameplay

#endif  // GRIDSHOTMODE_HPP
