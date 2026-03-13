/// @file TrackingTargetsMode.cpp
#include "gameplay/modes/TrackingTargetsMode.hpp"

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

constexpr double TRACKING_SPEED_DEFAULT = 2.5;
constexpr double TRACKING_SPEED_MIN = 2.5;
constexpr double TRACKING_SPEED_MAX = 10.0;
constexpr double TRACKING_SPEED_STEP = 0.1;
constexpr double AUTO_HIT_CHECK_INTERVAL_SECONDS = 0.05;

constexpr double DIRECTION_COMPONENT_MIN = 0.15;
constexpr double PI = 3.14159265358979323846;
}  // namespace

namespace xaimassist::gameplay {
TrackingTargetsMode::TrackingTargetsMode(
    core::EventBus& eventBus, core::Logger& logger,
    ISceneCommandSink& sceneCommandSink, TargetSystem& targetSystem,
    IGameplaySettingsProvider& settingsProvider)
    : GameMode(MetadataDefinition(), eventBus, logger, sceneCommandSink,
               targetSystem, settingsProvider),
      m_randomEngine(CreateSeededRandomEngine()) {}

GameModeMetadata TrackingTargetsMode::MetadataDefinition() {
    return GameModeMetadata{
        "tracking_targets",
        "Tracking Targets",
        "Spawns one moving sphere that continuously tracks within a bounded "
        "YZ area. On area bounds, movement bounces by reversing and "
        "randomizing direction while keeping speed. Scoring is automatic: "
        "every 50ms checks hit State (no left click).",
        60.0,
        25.0,
        0,
        0,
        0,
        {
            GameModeSettingMetadata{"speed", "Speed", TRACKING_SPEED_DEFAULT,
                                    TRACKING_SPEED_MIN, TRACKING_SPEED_MAX,
                                    TRACKING_SPEED_STEP, false},
        }};
}

void TrackingTargetsMode::OnStart() {
    m_activeTargetId = 0;
    m_activeTargetPosition = {0.0, 0.0, 0.0};
    m_activeMovementDirection = _RandomUnitPlanarDirection();
    m_activeMovementSpeed =
        std::clamp(ModeSettingValue("speed", TRACKING_SPEED_DEFAULT),
                   TRACKING_SPEED_MIN, TRACKING_SPEED_MAX);
    m_autoHitCheckAccumulatorSeconds = 0.0;

    _SpawnMovingTargetAtRandomSlot();

    GetLogger().Info("gameplay", "TrackingTargetsMode started");
}

void TrackingTargetsMode::OnUpdate(double dtSeconds) {
    if (m_activeTargetId == 0) {
        return;
    }

    if (!GetTargetSystem().HasTarget(m_activeTargetId)) {
        return;
    }

    const double safeDelta = std::max(0.0, dtSeconds);
    if (safeDelta <= 0.0) {
        return;
    }

    const double nextY =
        m_activeTargetPosition[1] +
        m_activeMovementDirection[0] * m_activeMovementSpeed * safeDelta;
    const double nextZ =
        m_activeTargetPosition[2] +
        m_activeMovementDirection[1] * m_activeMovementSpeed * safeDelta;

    const bool hitYBound = (nextY <= TARGET_Y_MIN) || (nextY >= TARGET_Y_MAX);
    const bool hitZBound = (nextZ <= TARGET_Z_MIN) || (nextZ >= TARGET_Z_MAX);

    m_activeTargetPosition[1] = std::clamp(nextY, TARGET_Y_MIN, TARGET_Y_MAX);
    m_activeTargetPosition[2] = std::clamp(nextZ, TARGET_Z_MIN, TARGET_Z_MAX);

    if (hitYBound || hitZBound) {
        _UpdateMovementDirectionAfterBounce(hitYBound, hitZBound);
    }

    if (!GetSceneCommandSink().UpdateTargetPosition(
            static_cast<SceneObjectId>(m_activeTargetId),
            m_activeTargetPosition)) {
        return;
    }

    m_autoHitCheckAccumulatorSeconds += safeDelta;
    while (m_autoHitCheckAccumulatorSeconds >= AUTO_HIT_CHECK_INTERVAL_SECONDS) {
        m_autoHitCheckAccumulatorSeconds -= AUTO_HIT_CHECK_INTERVAL_SECONDS;
        GetEventBus().Publish(core::events::ShotFiredEvent{
            SessionId(), std::chrono::steady_clock::now()});
    }
}

void TrackingTargetsMode::OnStop() {
    m_activeTargetId = 0;
    m_activeTargetPosition = {0.0, 0.0, 0.0};
    m_activeMovementDirection = {0.0, 1.0};
    m_activeMovementSpeed = TRACKING_SPEED_DEFAULT;
    m_autoHitCheckAccumulatorSeconds = 0.0;

    GetLogger().Info("gameplay", "TrackingTargetsMode stopped");
}

bool TrackingTargetsMode::_SpawnMovingTargetAtRandomSlot() {
    const GameplayRuntimeSettings runtimeSettings =
        GetSettingsProvider().CurrentSettings();
    const auto spawnOffset = _RandomSpawnOffset();

    TargetDefinition Definition;
    Definition.type = TargetType::Static;
    Definition.radius = runtimeSettings.TargetRadius;
    Definition.initialPosition = {std::max(0.1, ConfiguredDistance()),
                                  spawnOffset[0], spawnOffset[1]};
    Definition.color = runtimeSettings.targetColor;
    Definition.collidable = true;
    Definition.destroyOnHit = false;

    m_activeTargetId = GetTargetSystem().SpawnTarget(SessionId(), Definition);
    if (m_activeTargetId == 0) {
        GetLogger().Warning("gameplay",
                            "TrackingTargetsMode failed to spawn moving target");
        return false;
    }

    m_activeTargetPosition = Definition.initialPosition;
    m_activeMovementDirection = _RandomUnitPlanarDirection();
    m_activeMovementSpeed =
        std::clamp(ModeSettingValue("speed", TRACKING_SPEED_DEFAULT),
                   TRACKING_SPEED_MIN, TRACKING_SPEED_MAX);
    return true;
}

std::array<double, 2> TrackingTargetsMode::_RandomSpawnOffset() {
    std::uniform_real_distribution<double> yDistribution(TARGET_Y_MIN,
                                                         TARGET_Y_MAX);
    std::uniform_real_distribution<double> zDistribution(TARGET_Z_MIN,
                                                         TARGET_Z_MAX);
    return {
        yDistribution(m_randomEngine),
        zDistribution(m_randomEngine),
    };
}

std::array<double, 2> TrackingTargetsMode::_RandomUnitPlanarDirection() {
    std::uniform_real_distribution<double> angleDistribution(0.0, 2.0 * PI);
    const double angle = angleDistribution(m_randomEngine);
    return {std::cos(angle), std::sin(angle)};
}

void TrackingTargetsMode::_UpdateMovementDirectionAfterBounce(bool hitYBound,
                                                              bool hitZBound) {
    if (!hitYBound && !hitZBound) {
        return;
    }

    const std::array<double, 2> randomDirection = _RandomUnitPlanarDirection();
    std::array<double, 2> nextDirection = randomDirection;

    if (hitYBound) {
        const double reversedYSign =
            m_activeMovementDirection[0] >= 0.0 ? -1.0 : 1.0;
        nextDirection[0] = reversedYSign * std::max(DIRECTION_COMPONENT_MIN,
                                                    std::abs(randomDirection[0]));
    }

    if (hitZBound) {
        const double reversedZSign =
            m_activeMovementDirection[1] >= 0.0 ? -1.0 : 1.0;
        nextDirection[1] = reversedZSign * std::max(DIRECTION_COMPONENT_MIN,
                                                    std::abs(randomDirection[1]));
    }

    const double length = std::hypot(nextDirection[0], nextDirection[1]);
    if (length <= 0.000001) {
        if (hitYBound) {
            m_activeMovementDirection = {
                m_activeMovementDirection[0] >= 0.0 ? -1.0 : 1.0, 0.0};
            return;
        }

        m_activeMovementDirection = {
            0.0, m_activeMovementDirection[1] >= 0.0 ? -1.0 : 1.0};
        return;
    }

    m_activeMovementDirection = {nextDirection[0] / length,
                                 nextDirection[1] / length};
}
}  // namespace xaimassist::gameplay
