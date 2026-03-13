/// @file WorldRaycastHitTest.cpp
#include "app/WorldRaycastHitTest.hpp"

#include <array>
#include <cmath>
#include <limits>

#include "engine/Engine.hpp"
#include "world/World.hpp"

namespace {
constexpr double PI = 3.14159265358979323846;

double toRadians(double degrees) { return degrees * PI / 180.0; }

std::array<double, 3> cameraForwardFromEuler(double yawDegrees,
                                             double pitchDegrees) {
    const double yawRadians = toRadians(yawDegrees);
    const double pitchRadians = toRadians(pitchDegrees);

    const double x = std::cos(pitchRadians) * std::cos(yawRadians);
    const double y = std::sin(pitchRadians);
    const double z = std::cos(pitchRadians) * std::sin(yawRadians);

    return {x, y, z};
}

double dot(const std::array<double, 3>& left,
           const std::array<double, 3>& right) {
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

std::array<double, 3> subtract(const std::array<double, 3>& left,
                               const std::array<double, 3>& right) {
    return {left[0] - right[0], left[1] - right[1], left[2] - right[2]};
}

// Analytic ray-sphere intersection; returns nearest positive t or -1.
double intersectRaySphere(const std::array<double, 3>& origin,
                          const std::array<double, 3>& direction,
                          const std::array<double, 3>& center, double radius) {
    const auto oc = subtract(origin, center);
    const double b = dot(oc, direction);
    const double c = dot(oc, oc) - radius * radius;
    const double discriminant = b * b - c;
    if (discriminant < 0.0) {
        return -1.0;
    }

    const double root = std::sqrt(discriminant);
    const double nearDistance = -b - root;
    const double farDistance = -b + root;

    if (nearDistance > 0.0001) {
        return nearDistance;
    }

    if (farDistance > 0.0001) {
        return farDistance;
    }

    return -1.0;
}
}  // namespace

namespace xaimassist::app {
WorldRaycastHitTest::WorldRaycastHitTest(const world::World& world,
                                         const engine::Engine& engine)
    : m_world(world), m_engine(engine) {}

std::optional<gameplay::HitTestResult> WorldRaycastHitTest::CastShotRay(
    const core::events::ShotFiredEvent& shotFiredEvent) const {
    (void)shotFiredEvent;

    const auto CameraPose = m_engine.GetCameraPose();
    const std::array<double, 3> rayOrigin = CameraPose.position;
    const std::array<double, 3> rayDirection =
        cameraForwardFromEuler(CameraPose.yawDegrees, CameraPose.pitchDegrees);

    double closestDistance = std::numeric_limits<double>::infinity();
    gameplay::HitTestResult result;

    const auto Entities = m_world.Entities();
    for (const auto entityId : Entities) {
        const auto* transformComponent = m_world.Transform(entityId);
        const auto* colliderComponent = m_world.Collider(entityId);
        const auto* renderComponent = m_world.Render(entityId);

        if (transformComponent == nullptr || colliderComponent == nullptr ||
            renderComponent == nullptr) {
            continue;
        }

        if (!renderComponent->visible) {
            continue;
        }

        if (colliderComponent->shapeType != world::ColliderShapeType::Sphere) {
            continue;
        }

        const double hitDistance = intersectRaySphere(
            rayOrigin, rayDirection, transformComponent->position,
            colliderComponent->sphereRadius);

        if (hitDistance < 0.0 || hitDistance >= closestDistance) {
            continue;
        }

        closestDistance = hitDistance;
        result.TargetId = static_cast<std::uint64_t>(entityId);
    }

    if (result.TargetId == 0) {
        return std::nullopt;
    }

    return result;
}
}  // namespace xaimassist::app
