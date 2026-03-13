/// @file StrafingTargetsMode.cpp
#include "gameplay/modes/StrafingTargetsMode.hpp"

#include <algorithm>
#include <array>
#include <chrono>
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

constexpr double STRAFE_SPEED_DEFAULT = 2.5;
constexpr double STRAFE_SPEED_MIN = 2.5;
constexpr double STRAFE_SPEED_MAX = 10.0;
constexpr double STRAFE_SPEED_STEP = 0.1;
constexpr double MOVING_MISS_TIMEOUT_SECONDS = 2.5;
constexpr double PI = 3.14159265358979323846;
}  // namespace

namespace xaimassist::gameplay {
StrafingTargetsMode::StrafingTargetsMode(
    core::EventBus& eventBus, core::Logger& logger,
    ISceneCommandSink& sceneCommandSink, TargetSystem& targetSystem,
    IGameplaySettingsProvider& settingsProvider)
    : GameMode(MetadataDefinition(), eventBus, logger, sceneCommandSink,
               targetSystem, settingsProvider),
      m_randomEngine(CreateSeededRandomEngine()) {}

GameModeMetadata StrafingTargetsMode::MetadataDefinition() {
    return GameModeMetadata{
        "strafing_targets",
        "Strafing Targets",
        "Starts with one static sphere. First hit starts continuous movement "
        "on a random YZ axis at configured speed. If the moving sphere is not "
        "hit within 2.5 seconds it counts as miss and respawns as static.",
        60.0,
        25.0,
        0,
        0,
        0,
        {
            GameModeSettingMetadata{"speed", "Speed", STRAFE_SPEED_DEFAULT,
                                    STRAFE_SPEED_MIN, STRAFE_SPEED_MAX,
                                    STRAFE_SPEED_STEP, false},
        }};
}

void StrafingTargetsMode::OnStart() {
    m_activeTargetId = 0;
    m_previousStaticSpawnOffset.reset();
    m_activeTargetSpawnOffset.reset();
    m_activeTargetPosition = {0.0, 0.0, 0.0};
    m_activeMovementDirection = {0.0, 1.0};
    m_activeMovementSpeed = STRAFE_SPEED_DEFAULT;
    m_movingElapsedSeconds = 0.0;
    m_targetStage = TargetStage::StaticAwaitingActivation;

    _SpawnStaticTargetAtRandomSlot();

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

            if (!m_activeTargetSpawnOffset.has_value()) {
                m_targetStage = TargetStage::StaticAwaitingActivation;
                m_movingElapsedSeconds = 0.0;
                _SpawnStaticTargetAtRandomSlot();
                return;
            }

            if (m_targetStage == TargetStage::StaticAwaitingActivation) {
                const double speed =
                    std::max(STRAFE_SPEED_MIN,
                             ModeSettingValue("speed", STRAFE_SPEED_DEFAULT));

                if (!_SpawnMovingTargetAtOffset(*m_activeTargetSpawnOffset, speed)) {
                    m_targetStage = TargetStage::StaticAwaitingActivation;
                    m_movingElapsedSeconds = 0.0;
                    _SpawnStaticTargetAtRandomSlot();
                    return;
                }

                m_targetStage = TargetStage::MovingAwaitingHit;
                return;
            }

            m_targetStage = TargetStage::StaticAwaitingActivation;
            m_movingElapsedSeconds = 0.0;
            _SpawnStaticTargetAtRandomSlot();
        });

    GetLogger().Info("gameplay", "StrafingTargetsMode started");
}

void StrafingTargetsMode::OnUpdate(double dtSeconds) {
    if (m_targetStage != TargetStage::MovingAwaitingHit ||
        m_activeTargetId == 0) {
        return;
    }

    if (!GetTargetSystem().HasTarget(m_activeTargetId)) {
        return;
    }

    const double safeDelta = std::max(0.0, dtSeconds);
    m_activeTargetPosition[1] +=
        m_activeMovementDirection[0] * m_activeMovementSpeed * safeDelta;
    m_activeTargetPosition[2] +=
        m_activeMovementDirection[1] * m_activeMovementSpeed * safeDelta;

    if (!GetSceneCommandSink().UpdateTargetPosition(
            static_cast<SceneObjectId>(m_activeTargetId),
            m_activeTargetPosition)) {
        return;
    }

    m_movingElapsedSeconds += safeDelta;
    if (m_movingElapsedSeconds < MOVING_MISS_TIMEOUT_SECONDS) {
        return;
    }

    GetTargetSystem().ClearSessionTargets(SessionId());
    GetEventBus().Publish(core::events::ShotMissEvent{
        SessionId(), std::chrono::steady_clock::now()});

    m_targetStage = TargetStage::StaticAwaitingActivation;
    m_movingElapsedSeconds = 0.0;
    _SpawnStaticTargetAtRandomSlot();
}

void StrafingTargetsMode::OnStop() {
    if (m_shotHitSubscriptionId != 0) {
        GetEventBus().Unsubscribe(m_shotHitSubscriptionId);
        m_shotHitSubscriptionId = 0;
    }

    m_activeTargetId = 0;
    m_previousStaticSpawnOffset.reset();
    m_activeTargetSpawnOffset.reset();
    m_activeTargetPosition = {0.0, 0.0, 0.0};
    m_activeMovementDirection = {0.0, 1.0};
    m_activeMovementSpeed = STRAFE_SPEED_DEFAULT;
    m_movingElapsedSeconds = 0.0;
    m_targetStage = TargetStage::StaticAwaitingActivation;

    GetLogger().Info("gameplay", "StrafingTargetsMode stopped");
}

void StrafingTargetsMode::_SpawnStaticTargetAtRandomSlot() {
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
        m_activeTargetPosition = {0.0, 0.0, 0.0};
        m_activeTargetSpawnOffset.reset();
        GetLogger().Warning("gameplay",
                            "StrafingTargetsMode failed to spawn static target");
        return;
    }

    m_targetStage = TargetStage::StaticAwaitingActivation;
    m_movingElapsedSeconds = 0.0;
    m_activeTargetSpawnOffset = spawnOffset;
    m_previousStaticSpawnOffset = spawnOffset;
    m_activeTargetPosition = Definition.initialPosition;
}

bool StrafingTargetsMode::_SpawnMovingTargetAtOffset(
    const std::array<double, 2>& spawnOffset, double speed) {
    const GameplayRuntimeSettings runtimeSettings =
        GetSettingsProvider().CurrentSettings();
    const double clampedSpeed =
        std::clamp(speed, STRAFE_SPEED_MIN, STRAFE_SPEED_MAX);
    const std::array<double, 2> direction = _RandomUnitPlanarDirection();

    TargetDefinition Definition;
    Definition.type = TargetType::Static;
    Definition.radius = runtimeSettings.TargetRadius;
    Definition.initialPosition = {std::max(0.1, ConfiguredDistance()),
                                  spawnOffset[0], spawnOffset[1]};
    Definition.color = runtimeSettings.targetColor;
    Definition.collidable = true;

    m_activeTargetId = GetTargetSystem().SpawnTarget(SessionId(), Definition);
    if (m_activeTargetId == 0) {
        GetLogger().Warning("gameplay",
                            "StrafingTargetsMode failed to spawn moving target");
        return false;
    }

    m_activeTargetSpawnOffset = spawnOffset;
    m_activeTargetPosition = Definition.initialPosition;
    m_activeMovementDirection = direction;
    m_activeMovementSpeed = clampedSpeed;
    m_movingElapsedSeconds = 0.0;
    return true;
}

std::array<double, 2>
StrafingTargetsMode::_RandomSpawnOffset(double minimumDistance) {
    std::uniform_real_distribution<double> yDistribution(TARGET_Y_MIN,
                                                         TARGET_Y_MAX);
    std::uniform_real_distribution<double> zDistribution(TARGET_Z_MIN,
                                                         TARGET_Z_MAX);

    std::array<double, 2> bestOffset = {
        yDistribution(m_randomEngine),
        zDistribution(m_randomEngine),
    };

    if (!m_previousStaticSpawnOffset.has_value()) {
        return bestOffset;
    }

    double bestDistance =
        _PlanarDistance(bestOffset, *m_previousStaticSpawnOffset);
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
            _PlanarDistance(candidateOffset, *m_previousStaticSpawnOffset);
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

std::array<double, 2> StrafingTargetsMode::_RandomUnitPlanarDirection() {
    std::uniform_real_distribution<double> angleDistribution(0.0, 2.0 * PI);
    const double angle = angleDistribution(m_randomEngine);
    return {std::cos(angle), std::sin(angle)};
}

double
StrafingTargetsMode::_PlanarDistance(const std::array<double, 2>& left,
                                     const std::array<double, 2>& right) {
    const double deltaY = left[0] - right[0];
    const double deltaZ = left[1] - right[1];
    return std::sqrt(deltaY * deltaY + deltaZ * deltaZ);
}
}  // namespace xaimassist::gameplay
