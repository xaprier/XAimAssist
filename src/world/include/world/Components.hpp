/**
 * @file Components.hpp
 * @brief Lightweight data components attached to world entities.
 */

#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include <array>

namespace xaimassist::world {

/// Spatial placement (position, rotation, scale) of an entity.
struct TransformComponent {
    std::array<double, 3> position{0.0, 0.0, 0.0};
    std::array<double, 3> rotationEulerDegrees{0.0, 0.0, 0.0};
    std::array<double, 3> scale{1.0, 1.0, 1.0};
};

enum class RenderPrimitiveType { Sphere };

/// Visual representation metadata for an entity.
struct RenderComponent {
    RenderPrimitiveType primitiveType{RenderPrimitiveType::Sphere};
    std::array<double, 3> color{0.95, 0.35, 0.25};
    double sphereRadius{0.5};
    double opacity{1.0};
    bool visible{true};
};

enum class ColliderShapeType { Sphere };

/// Collision volume for hit-testing against an entity.
struct ColliderComponent {
    ColliderShapeType shapeType{ColliderShapeType::Sphere};
    double sphereRadius{0.5};
    bool isTrigger{false};
};
}  // namespace xaimassist::world

#endif  // COMPONENTS_HPP
