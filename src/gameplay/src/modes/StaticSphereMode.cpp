/// @file StaticSphereMode.cpp
#include "gameplay/modes/StaticSphereMode.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "core/Logger.hpp"
#include "gameplay/RandomEngineFactory.hpp"
#include "gameplay/TargetSystem.hpp"
#include "gameplay/targets/Target.hpp"

namespace {
constexpr double TARGET_Y_MIN = -2.5;
constexpr double TARGET_Y_MAX = 2.5;
constexpr double TARGET_Z_MIN = -2.5;
constexpr double TARGET_Z_MAX = 2.5;
constexpr std::size_t MAX_RANDOM_SAMPLE_ATTEMPTS = 32;
constexpr double MIN_SPAWN_DISTANCE_CAP_FACTOR = 0.60;
}  // namespace

namespace xaimassist::gameplay {
StaticSphereMode::StaticSphereMode(core::EventBus& eventBus,
                                   core::Logger& logger,
                                   ISceneCommandSink& sceneCommandSink,
                                   TargetSystem& targetSystem,
                                   IGameplaySettingsProvider& settingsProvider)
    : GameMode(MetadataDefinition(), eventBus, logger, sceneCommandSink,
               targetSystem, settingsProvider),
      m_randomEngine(CreateSeededRandomEngine()) {}

GameModeMetadata StaticSphereMode::MetadataDefinition() {
    return GameModeMetadata{
        "static_sphere", "Static Sphere",
        "Spawns one static sphere target and respawns it at a new slot on hit.",
        60.0, 25.0};
}

void StaticSphereMode::OnStart() {
    m_previousSpawnOffset.reset();

    _SpawnTargetAtCurrentSlot();

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

            if (shotHitEvent->targetId != m_activeTargetId) {
                return;
            }

            _SpawnTargetAtCurrentSlot();
        });

    GetLogger().Info("gameplay", "StaticSphereMode started");
}

void StaticSphereMode::OnUpdate(double dtSeconds) { (void)dtSeconds; }

void StaticSphereMode::OnStop() {
    if (m_shotHitSubscriptionId != 0) {
        GetEventBus().Unsubscribe(m_shotHitSubscriptionId);
        m_shotHitSubscriptionId = 0;
    }

    m_activeTargetId = 0;
    m_previousSpawnOffset.reset();

    GetLogger().Info("gameplay", "StaticSphereMode stopped");
}

void StaticSphereMode::_SpawnTargetAtCurrentSlot() {
    const GameplayRuntimeSettings runtimeSettings =
        GetSettingsProvider().CurrentSettings();

    TargetDefinition Definition;
    Definition.type = TargetType::Static;
    Definition.radius = runtimeSettings.TargetRadius;

    const double spawnDistanceX = std::max(0.1, ConfiguredDistance());
    const double spawnRangeY = TARGET_Y_MAX - TARGET_Y_MIN;
    const double spawnRangeZ = TARGET_Z_MAX - TARGET_Z_MIN;
    const double maxSpawnOffsetDistance =
        std::sqrt(spawnRangeY * spawnRangeY + spawnRangeZ * spawnRangeZ);
    const double requestedMinimumSpawnOffsetDistance = spawnDistanceX / 5.0;
    const double cappedMinimumSpawnOffsetDistance =
        std::min(requestedMinimumSpawnOffsetDistance,
                 maxSpawnOffsetDistance * MIN_SPAWN_DISTANCE_CAP_FACTOR);
    const double _MinimumSpawnOffsetDistance =
        std::clamp(cappedMinimumSpawnOffsetDistance, 0.0, maxSpawnOffsetDistance);

    const auto spawnOffset = _RandomSpawnOffset(_MinimumSpawnOffsetDistance);
    Definition.initialPosition = {spawnDistanceX, spawnOffset[0], spawnOffset[1]};
    Definition.color = runtimeSettings.targetColor;
    Definition.collidable = true;

    m_activeTargetId = GetTargetSystem().SpawnTarget(SessionId(), Definition);
    if (m_activeTargetId == 0) {
        GetLogger().Warning("gameplay", "StaticSphereMode failed to spawn target");
        return;
    }

    m_previousSpawnOffset = spawnOffset;
}

std::array<double, 2>
StaticSphereMode::_RandomSpawnOffset(double minimumDistance) {
    std::uniform_real_distribution<double> yDistribution(TARGET_Y_MIN,
                                                         TARGET_Y_MAX);
    std::uniform_real_distribution<double> zDistribution(TARGET_Z_MIN,
                                                         TARGET_Z_MAX);

    std::array<double, 2> bestOffset = {
        yDistribution(m_randomEngine),
        zDistribution(m_randomEngine),
    };

    if (!m_previousSpawnOffset.has_value()) {
        return bestOffset;
    }

    double bestDistance = _PlanarDistance(bestOffset, *m_previousSpawnOffset);
    if (bestDistance >= minimumDistance) {
        return bestOffset;
    }

    for (std::size_t attempt = 0; attempt < MAX_RANDOM_SAMPLE_ATTEMPTS;
         ++attempt) {
        const std::array<double, 2> candidateOffset = {
            yDistribution(m_randomEngine),
            zDistribution(m_randomEngine),
        };

        const double candidateDistance =
            _PlanarDistance(candidateOffset, *m_previousSpawnOffset);
        if (candidateDistance >= minimumDistance) {
            return candidateOffset;
        }

        if (candidateDistance > bestDistance) {
            bestDistance = candidateDistance;
            bestOffset = candidateOffset;
        }
    }

    return bestOffset;
}

double StaticSphereMode::_PlanarDistance(const std::array<double, 2>& left,
                                         const std::array<double, 2>& right) {
    const double deltaY = left[0] - right[0];
    const double deltaZ = left[1] - right[1];
    return std::sqrt(deltaY * deltaY + deltaZ * deltaZ);
}
}  // namespace xaimassist::gameplay
