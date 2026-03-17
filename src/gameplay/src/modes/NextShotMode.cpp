/// @file NextShotMode.cpp
#include "gameplay/modes/NextShotMode.hpp"

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
constexpr double PREVIEW_TARGET_OPACITY = 0.25;
}  // namespace

namespace xaimassist::gameplay {
NextShotMode::NextShotMode(core::EventBus& eventBus, core::Logger& logger,
                           ISceneCommandSink& sceneCommandSink,
                           TargetSystem& targetSystem,
                           IGameplaySettingsProvider& settingsProvider)
    : GameMode(MetadataDefinition(), eventBus, logger, sceneCommandSink,
               targetSystem, settingsProvider),
      m_randomEngine(CreateSeededRandomEngine()) {}

GameModeMetadata NextShotMode::MetadataDefinition() {
    return GameModeMetadata{
        "next_shot", "Next Shot",
        "Works like Static Sphere, but always shows where the next target will "
        "spawn as a low-opacity preview sphere.",
        60.0, 25.0};
}

void NextShotMode::OnStart() {
    m_activeTargetId = 0;
    m_previewTargetObjectId = 0;
    m_activeSpawnOffset.reset();
    m_previewSpawnOffset.reset();

    const double minimumDistance = _MinimumSpawnOffsetDistance();
    const auto activeSpawnOffset = _RandomSpawnOffset(minimumDistance, std::nullopt);
    if (!_SpawnActiveTargetAtOffset(activeSpawnOffset)) {
        GetLogger().Warning("gameplay",
                            "NextShotMode failed to spawn active target");
        return;
    }

    const auto previewSpawnOffset = _RandomSpawnOffset(minimumDistance, activeSpawnOffset);
    if (!_SpawnPreviewTargetAtOffset(previewSpawnOffset)) {
        GetLogger().Warning("gameplay",
                            "NextShotMode failed to spawn preview target");
    }

    m_shotHitSubscriptionId =
        GetEventBus().Subscribe([this](const core::events::CoreEvent& event) {
            const auto* shotHitEvent = std::get_if<core::events::ShotHitEvent>(&event);
            if (shotHitEvent == nullptr) {
                return;
            }

            if (shotHitEvent->sessionId != SessionId()) {
                return;
            }

            if (shotHitEvent->targetId != m_activeTargetId) {
                return;
            }

            if (!m_previewSpawnOffset.has_value()) {
                const double minimumDistance = _MinimumSpawnOffsetDistance();
                const auto fallbackActive = _RandomSpawnOffset(minimumDistance, m_activeSpawnOffset);
                if (!_SpawnActiveTargetAtOffset(fallbackActive)) {
                    return;
                }

                const auto fallbackPreview = _RandomSpawnOffset(minimumDistance, fallbackActive);
                _SpawnPreviewTargetAtOffset(fallbackPreview);
                return;
            }

            const auto nextActiveOffset = *m_previewSpawnOffset;
            if (m_previewTargetObjectId != 0) {
                GetSceneCommandSink().DestroyTarget(m_previewTargetObjectId);
                m_previewTargetObjectId = 0;
            }
            m_previewSpawnOffset.reset();

            if (!_SpawnActiveTargetAtOffset(nextActiveOffset)) {
                GetLogger().Warning("gameplay",
                                    "NextShotMode failed to promote preview to active target");
                return;
            }

            const double minimumDistance = _MinimumSpawnOffsetDistance();
            const auto nextPreviewOffset = _RandomSpawnOffset(minimumDistance, nextActiveOffset);
            if (!_SpawnPreviewTargetAtOffset(nextPreviewOffset)) {
                GetLogger().Warning("gameplay",
                                    "NextShotMode failed to spawn next preview target");
            }
        });

    GetLogger().Info("gameplay", "NextShotMode started");
}

void NextShotMode::OnUpdate(double dtSeconds) { (void)dtSeconds; }

void NextShotMode::OnStop() {
    if (m_shotHitSubscriptionId != 0) {
        GetEventBus().Unsubscribe(m_shotHitSubscriptionId);
        m_shotHitSubscriptionId = 0;
    }

    if (m_previewTargetObjectId != 0) {
        GetSceneCommandSink().DestroyTarget(m_previewTargetObjectId);
        m_previewTargetObjectId = 0;
    }

    m_activeTargetId = 0;
    m_activeSpawnOffset.reset();
    m_previewSpawnOffset.reset();

    GetLogger().Info("gameplay", "NextShotMode stopped");
}

bool NextShotMode::_SpawnActiveTargetAtOffset(const std::array<double, 2>& spawnOffset) {
    const GameplayRuntimeSettings runtimeSettings = GetSettingsProvider().CurrentSettings();

    TargetDefinition definition;
    definition.type = TargetType::Static;
    definition.radius = runtimeSettings.TargetRadius;
    definition.initialPosition = {std::max(0.1, ConfiguredDistance()),
                                  spawnOffset[0], spawnOffset[1]};
    definition.color = runtimeSettings.targetColor;
    definition.collidable = true;

    m_activeTargetId = GetTargetSystem().SpawnTarget(SessionId(), definition);
    if (m_activeTargetId == 0) {
        GetLogger().Warning("gameplay",
                            "NextShotMode failed to spawn active target");
        return false;
    }

    m_activeSpawnOffset = spawnOffset;
    return true;
}

bool NextShotMode::_SpawnPreviewTargetAtOffset(const std::array<double, 2>& spawnOffset) {
    if (m_previewTargetObjectId != 0) {
        GetSceneCommandSink().DestroyTarget(m_previewTargetObjectId);
        m_previewTargetObjectId = 0;
    }

    const GameplayRuntimeSettings runtimeSettings = GetSettingsProvider().CurrentSettings();

    SphereTargetSpawnRequest previewRequest;
    previewRequest.radius = runtimeSettings.TargetRadius;
    previewRequest.position = {std::max(0.1, ConfiguredDistance()),
                               spawnOffset[0], spawnOffset[1]};
    previewRequest.color = runtimeSettings.targetColor;
    previewRequest.opacity = PREVIEW_TARGET_OPACITY;
    previewRequest.collidable = false;

    m_previewTargetObjectId = GetSceneCommandSink().SpawnSphereTarget(previewRequest);
    if (m_previewTargetObjectId == 0) {
        m_previewSpawnOffset.reset();
        GetLogger().Warning("gameplay",
                            "NextShotMode failed to spawn preview target");
        return false;
    }

    m_previewSpawnOffset = spawnOffset;
    return true;
}

std::array<double, 2> NextShotMode::_RandomSpawnOffset(
    double minimumDistance,
    const std::optional<std::array<double, 2>>& avoidOffset) {
    std::uniform_real_distribution<double> yDistribution(TARGET_Y_MIN,
                                                         TARGET_Y_MAX);
    std::uniform_real_distribution<double> zDistribution(TARGET_Z_MIN,
                                                         TARGET_Z_MAX);

    std::array<double, 2> bestOffset = {
        yDistribution(m_randomEngine),
        zDistribution(m_randomEngine),
    };

    if (!avoidOffset.has_value()) {
        return bestOffset;
    }

    double bestDistance = _PlanarDistance(bestOffset, *avoidOffset);
    if (bestDistance >= minimumDistance) {
        return bestOffset;
    }

    for (std::size_t attempt = 0; attempt < MAX_RANDOM_SAMPLE_ATTEMPTS; ++attempt) {
        const std::array<double, 2> candidateOffset = {
            yDistribution(m_randomEngine),
            zDistribution(m_randomEngine),
        };

        const double candidateDistance = _PlanarDistance(candidateOffset, *avoidOffset);
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

double NextShotMode::_MinimumSpawnOffsetDistance() const {
    const double spawnDistanceX = std::max(0.1, ConfiguredDistance());
    const double spawnRangeY = TARGET_Y_MAX - TARGET_Y_MIN;
    const double spawnRangeZ = TARGET_Z_MAX - TARGET_Z_MIN;
    const double maxSpawnOffsetDistance = std::sqrt(spawnRangeY * spawnRangeY + spawnRangeZ * spawnRangeZ);
    const double requestedMinimumSpawnOffsetDistance = spawnDistanceX / 5.0;
    const double cappedMinimumSpawnOffsetDistance = std::min(requestedMinimumSpawnOffsetDistance, maxSpawnOffsetDistance * MIN_SPAWN_DISTANCE_CAP_FACTOR);
    return std::clamp(cappedMinimumSpawnOffsetDistance, 0.0, maxSpawnOffsetDistance);
}

double NextShotMode::_PlanarDistance(const std::array<double, 2>& left,
                                     const std::array<double, 2>& right) {
    const double deltaY = left[0] - right[0];
    const double deltaZ = left[1] - right[1];
    return std::sqrt(deltaY * deltaY + deltaZ * deltaZ);
}
}  // namespace xaimassist::gameplay
