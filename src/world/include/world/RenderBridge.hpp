/**
 * @file RenderBridge.hpp
 * @brief Abstract interface between the world layer and a render backend.
 */

#ifndef RENDERBRIDGE_HPP
#define RENDERBRIDGE_HPP

#include <cstdint>

#include "world/Components.hpp"

namespace xaimassist::world {

/**
 * @class IRenderSceneBackend
 * @brief Backend-agnostic interface for creating/updating/destroying
 *        renderable objects from world component data.
 */
class IRenderSceneBackend {
  public:
    using RenderObjectId = std::uint64_t;

    virtual ~IRenderSceneBackend() = default;

    /// Create a sphere render object from component data; return its id.
    virtual RenderObjectId CreateSphere(const TransformComponent& Transform,
                                        const RenderComponent& Render) = 0;

    /// Update an existing sphere's transform and appearance.
    virtual bool UpdateSphere(RenderObjectId objectId,
                              const TransformComponent& Transform,
                              const RenderComponent& Render) = 0;

    /// Destroy a render object by id.
    virtual bool DestroyObject(RenderObjectId objectId) = 0;
};
}  // namespace xaimassist::world

#endif  // RENDERBRIDGE_HPP
