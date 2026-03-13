/// @file Target.cpp
#include "gameplay/targets/Target.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
double vectorLength(const std::array<double, 3>& vector) {
    return std::sqrt(vector[0] * vector[0] + vector[1] * vector[1] +
                     vector[2] * vector[2]);
}

std::array<double, 3> normalizeOrDefault(const std::array<double, 3>& vector) {
    const double length = vectorLength(vector);
    if (length <= 0.000001) {
        return {1.0, 0.0, 0.0};
    }

    return {vector[0] / length, vector[1] / length, vector[2] / length};
}
}  // namespace

namespace xaimassist::gameplay {
Target::Target(std::uint64_t TargetId, std::uint64_t SessionId,
               SceneObjectId sceneObjId, TargetDefinition definition,
               std::chrono::steady_clock::time_point SpawnedAt)
    : m_targetId(TargetId), m_sessionId(SessionId), m_sceneObjectId(sceneObjId), m_definition(std::move(definition)), m_spawnedAt(SpawnedAt), m_currentPosition(m_definition.initialPosition) {}

bool Target::OnHit(ISceneCommandSink& sceneCommandSink) {
    if (m_state == TargetState::Destroyed) {
        return false;
    }

    if (!m_definition.destroyOnHit) {
        return true;
    }

    m_state = TargetState::Destroyed;
    return sceneCommandSink.DestroyTarget(m_sceneObjectId);
}

bool Target::SetColor(ISceneCommandSink& sceneCommandSink,
                      const std::array<double, 3>& color) {
    if (m_state == TargetState::Destroyed) {
        return false;
    }

    if (!sceneCommandSink.UpdateTargetColor(m_sceneObjectId, color)) {
        return false;
    }

    m_definition.color = color;
    return true;
}

std::uint64_t Target::TargetId() const noexcept { return m_targetId; }

std::uint64_t Target::SessionId() const noexcept { return m_sessionId; }

SceneObjectId Target::GetSceneObjectId() const noexcept {
    return m_sceneObjectId;
}

TargetState Target::State() const noexcept { return m_state; }

std::chrono::steady_clock::time_point Target::SpawnedAt() const noexcept {
    return m_spawnedAt;
}

const TargetDefinition& Target::Definition() const noexcept {
    return m_definition;
}

const std::array<double, 3>& Target::CurrentPosition() const noexcept {
    return m_currentPosition;
}

bool Target::ApplyPosition(ISceneCommandSink& sceneCommandSink,
                           const std::array<double, 3>& newPosition) {
    if (m_state == TargetState::Destroyed) {
        return false;
    }

    if (!sceneCommandSink.UpdateTargetPosition(m_sceneObjectId, newPosition)) {
        return false;
    }

    m_currentPosition = newPosition;
    return true;
}

bool Target::ApplyVisibility(ISceneCommandSink& sceneCommandSink,
                             bool visible) {
    if (m_state == TargetState::Destroyed) {
        return false;
    }

    return sceneCommandSink.UpdateTargetVisibility(m_sceneObjectId, visible);
}

void StaticTarget::Update(double dtSeconds,
                          ISceneCommandSink& sceneCommandSink) {
    (void)dtSeconds;
    (void)sceneCommandSink;
}

MovingTarget::MovingTarget(std::uint64_t TargetId, std::uint64_t SessionId,
                           SceneObjectId sceneObjId,
                           TargetDefinition definition,
                           std::chrono::steady_clock::time_point SpawnedAt)
    : Target(TargetId, SessionId, sceneObjId, std::move(definition), SpawnedAt),
      m_anchorPosition(CurrentPosition()) {
    const auto& velocity = this->Definition().movementVelocity;
    m_speed = std::max(0.1, vectorLength(velocity));
    m_axis = normalizeOrDefault(velocity);
    m_extent = std::max(0.1, this->Definition().movementExtent);
}

void MovingTarget::Update(double dtSeconds,
                          ISceneCommandSink& sceneCommandSink) {
    const double safeDelta = std::max(0.0, dtSeconds);

    m_offset += m_direction * m_speed * safeDelta;
    if (m_offset > m_extent) {
        m_offset = m_extent;
        m_direction = -1.0;
    } else if (m_offset < -m_extent) {
        m_offset = -m_extent;
        m_direction = 1.0;
    }

    std::array<double, 3> nextPosition{
        m_anchorPosition[0] + m_axis[0] * m_offset,
        m_anchorPosition[1] + m_axis[1] * m_offset,
        m_anchorPosition[2] + m_axis[2] * m_offset,
    };

    ApplyPosition(sceneCommandSink, nextPosition);
}

StrafingTarget::StrafingTarget(std::uint64_t TargetId, std::uint64_t SessionId,
                               SceneObjectId sceneObjId,
                               TargetDefinition definition,
                               std::chrono::steady_clock::time_point SpawnedAt)
    : Target(TargetId, SessionId, sceneObjId, std::move(definition), SpawnedAt),
      m_anchorPosition(CurrentPosition()) {
    m_extent = std::max(0.1, this->Definition().movementExtent);
    m_angularSpeed =
        std::max(0.8, std::abs(this->Definition().movementVelocity[0]) * 2.0);
}

void StrafingTarget::Update(double dtSeconds,
                            ISceneCommandSink& sceneCommandSink) {
    const double safeDelta = std::max(0.0, dtSeconds);
    m_elapsedSeconds += safeDelta;

    const double offset = std::sin(m_elapsedSeconds * m_angularSpeed) * m_extent;
    std::array<double, 3> nextPosition{
        m_anchorPosition[0] + offset,
        m_anchorPosition[1],
        m_anchorPosition[2],
    };

    ApplyPosition(sceneCommandSink, nextPosition);
}

BlinkingTarget::BlinkingTarget(std::uint64_t TargetId, std::uint64_t SessionId,
                               SceneObjectId sceneObjId,
                               TargetDefinition definition,
                               std::chrono::steady_clock::time_point SpawnedAt)
    : Target(TargetId, SessionId, sceneObjId, std::move(definition),
             SpawnedAt) {
    m_visibleSeconds = std::max(0.1, this->Definition().blinkVisibleSeconds);
    m_hiddenSeconds = std::max(0.1, this->Definition().blinkHiddenSeconds);
}

void BlinkingTarget::Update(double dtSeconds,
                            ISceneCommandSink& sceneCommandSink) {
    const double safeDelta = std::max(0.0, dtSeconds);
    m_phaseSeconds += safeDelta;

    if (m_visible) {
        if (m_phaseSeconds < m_visibleSeconds) {
            return;
        }

        m_phaseSeconds = 0.0;
        m_visible = false;
        ApplyVisibility(sceneCommandSink, false);
        return;
    }

    if (m_phaseSeconds < m_hiddenSeconds) {
        return;
    }

    m_phaseSeconds = 0.0;
    m_visible = true;
    ApplyVisibility(sceneCommandSink, true);
}
}  // namespace xaimassist::gameplay
