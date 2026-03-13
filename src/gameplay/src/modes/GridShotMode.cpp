/// @file GridShotMode.cpp
#include "gameplay/modes/GridShotMode.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "core/Logger.hpp"
#include "gameplay/RandomEngineFactory.hpp"
#include "gameplay/TargetSystem.hpp"
#include "gameplay/targets/Target.hpp"

namespace {
constexpr int GRID_DEFAULT_ROW_COUNT = 5;
constexpr int GRID_DEFAULT_COLUMN_COUNT = 5;
constexpr int GRID_DEFAULT_ACTIVE_TARGET_COUNT = 3;
constexpr int GRID_MIN_AXIS_COUNT = 2;
constexpr int GRID_MAX_AXIS_COUNT = 15;

std::size_t clampedAxisCount(int requestedCount, int fallbackCount) {
    const int positiveRequested =
        requestedCount > 0 ? requestedCount : fallbackCount;
    return static_cast<std::size_t>(
        std::clamp(positiveRequested, GRID_MIN_AXIS_COUNT, GRID_MAX_AXIS_COUNT));
}
}  // namespace

namespace xaimassist::gameplay {
GridShotMode::GridShotMode(core::EventBus& eventBus, core::Logger& logger,
                           ISceneCommandSink& sceneCommandSink,
                           TargetSystem& targetSystem,
                           IGameplaySettingsProvider& settingsProvider)
    : GameMode(MetadataDefinition(), eventBus, logger, sceneCommandSink,
               targetSystem, settingsProvider),
      m_randomEngine(CreateSeededRandomEngine()) {}

GameModeMetadata GridShotMode::MetadataDefinition() {
    return GameModeMetadata{
        "gridshot",
        "GridShot",
        "Maintains three targets on a 5x5 fixed-distance grid and respawns "
        "hits at random free cells.",
        60.0,
        25.0,
        GRID_DEFAULT_ROW_COUNT,
        GRID_DEFAULT_COLUMN_COUNT,
        GRID_DEFAULT_ACTIVE_TARGET_COUNT,
        {
            GameModeSettingMetadata{"grid_rows", "Grid Rows",
                                    static_cast<double>(GRID_DEFAULT_ROW_COUNT),
                                    static_cast<double>(GRID_MIN_AXIS_COUNT),
                                    static_cast<double>(GRID_MAX_AXIS_COUNT), 1.0,
                                    true},
            GameModeSettingMetadata{
                "grid_columns", "Grid Columns",
                static_cast<double>(GRID_DEFAULT_COLUMN_COUNT),
                static_cast<double>(GRID_MIN_AXIS_COUNT),
                static_cast<double>(GRID_MAX_AXIS_COUNT), 1.0, true},
            GameModeSettingMetadata{
                "active_target_count", "Active Targets",
                static_cast<double>(GRID_DEFAULT_ACTIVE_TARGET_COUNT), 1.0,
                static_cast<double>(GRID_MAX_AXIS_COUNT * GRID_MAX_AXIS_COUNT -
                                    1),
                1.0, true},
        }};
}

void GridShotMode::ApplyModeSettings(
    const std::unordered_map<std::string, double>& requestedSettings) {
    GameMode::ApplyModeSettings(requestedSettings);

    const int rows = std::clamp(
        static_cast<int>(std::lround(ModeSettingValue(
            "grid_rows", static_cast<double>(GRID_DEFAULT_ROW_COUNT)))),
        GRID_MIN_AXIS_COUNT, GRID_MAX_AXIS_COUNT);
    const int columns = std::clamp(
        static_cast<int>(std::lround(ModeSettingValue(
            "grid_columns", static_cast<double>(GRID_DEFAULT_COLUMN_COUNT)))),
        GRID_MIN_AXIS_COUNT, GRID_MAX_AXIS_COUNT);

    const int maxActiveTargets = std::max(1, rows * columns - 1);
    const int activeTargets =
        std::clamp(static_cast<int>(std::lround(ModeSettingValue(
                       "active_target_count",
                       static_cast<double>(GRID_DEFAULT_ACTIVE_TARGET_COUNT)))),
                   1, maxActiveTargets);

    auto normalizedSettings = ModeSettings();
    normalizedSettings["grid_rows"] = static_cast<double>(rows);
    normalizedSettings["grid_columns"] = static_cast<double>(columns);
    normalizedSettings["active_target_count"] =
        static_cast<double>(activeTargets);

    GameMode::ApplyModeSettings(normalizedSettings);
}

void GridShotMode::OnStart() {
    m_gridRowCount = clampedAxisCount(
        static_cast<int>(std::lround(ModeSettingValue(
            "grid_rows", static_cast<double>(GRID_DEFAULT_ROW_COUNT)))),
        GRID_DEFAULT_ROW_COUNT);
    m_gridColumnCount = clampedAxisCount(
        static_cast<int>(std::lround(ModeSettingValue(
            "grid_columns", static_cast<double>(GRID_DEFAULT_COLUMN_COUNT)))),
        GRID_DEFAULT_COLUMN_COUNT);

    const std::size_t gridCellCount = m_gridRowCount * m_gridColumnCount;
    const std::size_t maxActiveTargetCount =
        gridCellCount > 1 ? (gridCellCount - 1) : 1;

    const int requestedActiveTargetCount = static_cast<int>(std::lround(
        ModeSettingValue("active_target_count",
                         static_cast<double>(GRID_DEFAULT_ACTIVE_TARGET_COUNT))));
    m_activeTargetCount = static_cast<std::size_t>(std::clamp(
        requestedActiveTargetCount, 1,
        static_cast<int>(std::max<std::size_t>(1, maxActiveTargetCount))));

    m_activeTargetCellById.clear();

    for (std::size_t targetIndex = 0; targetIndex < m_activeTargetCount;
         ++targetIndex) {
        _SpawnTargetAtRandomAvailableCell();
    }

    m_shotHitSubscriptionId =
        GetEventBus().Subscribe([this](const core::events::CoreEvent& event) {
            const auto* shotHitEvent =
                std::get_if<core::events::ShotHitEvent>(&event);
            if (shotHitEvent == nullptr) {
                return;
            }

            if (shotHitEvent->sessionId != SessionId()) {
                return;
            }

            const auto destroyedTargetIterator =
                m_activeTargetCellById.find(shotHitEvent->targetId);
            if (destroyedTargetIterator == m_activeTargetCellById.end()) {
                return;
            }

            const std::size_t destroyedCellIndex = destroyedTargetIterator->second;
            m_activeTargetCellById.erase(destroyedTargetIterator);
            _SpawnTargetAtRandomAvailableCell(destroyedCellIndex);
        });

    GetLogger().Info("gameplay", "GridShotMode started");
}

void GridShotMode::OnUpdate(double dtSeconds) { (void)dtSeconds; }

void GridShotMode::OnStop() {
    if (m_shotHitSubscriptionId != 0) {
        GetEventBus().Unsubscribe(m_shotHitSubscriptionId);
        m_shotHitSubscriptionId = 0;
    }

    m_activeTargetCellById.clear();
    GetLogger().Info("gameplay", "GridShotMode stopped");
}

void GridShotMode::_SpawnTargetAtRandomAvailableCell(
    std::optional<std::size_t> excludedCellIndex) {
    const auto availableCellIndex = _RandomAvailableCellIndex(excludedCellIndex);
    if (!availableCellIndex.has_value()) {
        GetLogger().Warning("gameplay",
                            "GridShotMode has no available cell for target spawn");
        return;
    }

    const GameplayRuntimeSettings runtimeSettings =
        GetSettingsProvider().CurrentSettings();

    TargetDefinition Definition;
    Definition.type = TargetType::Static;
    Definition.radius = runtimeSettings.TargetRadius;
    Definition.initialPosition = _WorldPositionForCell(*availableCellIndex);
    Definition.color = runtimeSettings.targetColor;
    Definition.collidable = true;

    const std::uint64_t TargetId =
        GetTargetSystem().SpawnTarget(SessionId(), Definition);
    if (TargetId == 0) {
        GetLogger().Warning("gameplay", "GridShotMode failed to spawn target");
        return;
    }

    m_activeTargetCellById.emplace(TargetId, *availableCellIndex);
}

std::optional<std::size_t> GridShotMode::_RandomAvailableCellIndex(
    std::optional<std::size_t> excludedCellIndex) {
    const std::size_t gridCellCount = m_gridRowCount * m_gridColumnCount;
    if (m_activeTargetCellById.size() >= gridCellCount) {
        return std::nullopt;
    }

    std::vector<std::size_t> availableCells;
    availableCells.reserve(gridCellCount - m_activeTargetCellById.size());

    for (std::size_t cellIndex = 0; cellIndex < gridCellCount; ++cellIndex) {
        if (excludedCellIndex.has_value() &&
            cellIndex == excludedCellIndex.value()) {
            continue;
        }

        if (!_IsCellOccupied(cellIndex)) {
            availableCells.push_back(cellIndex);
        }
    }

    if (availableCells.empty()) {
        return std::nullopt;
    }

    std::uniform_int_distribution<std::size_t> indexDistribution(
        0, availableCells.size() - 1);
    return availableCells[indexDistribution(m_randomEngine)];
}

std::array<double, 3>
GridShotMode::_WorldPositionForCell(std::size_t cellIndex) const {
    const std::size_t rowIndex = cellIndex / m_gridColumnCount;
    const std::size_t columnIndex = cellIndex % m_gridColumnCount;

    const double spawnDistanceX = std::max(0.1, ConfiguredDistance());
    const double cellSpacing = std::max(0.1, spawnDistanceX / 10.0);

    const double rowCenter = (static_cast<double>(m_gridRowCount) - 1.0) * 0.5;
    const double columnCenter =
        (static_cast<double>(m_gridColumnCount) - 1.0) * 0.5;

    const double offsetY =
        (static_cast<double>(columnIndex) - columnCenter) * cellSpacing;
    const double offsetZ =
        (rowCenter - static_cast<double>(rowIndex)) * cellSpacing;

    return {spawnDistanceX, offsetY, offsetZ};
}

bool GridShotMode::_IsCellOccupied(std::size_t cellIndex) const {
    return std::any_of(
        m_activeTargetCellById.begin(), m_activeTargetCellById.end(),
        [cellIndex](const auto& entry) { return entry.second == cellIndex; });
}
}  // namespace xaimassist::gameplay
