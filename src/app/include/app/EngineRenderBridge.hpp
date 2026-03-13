/**
 * @file EngineRenderBridge.hpp
 * @brief Adapts Engine sphere operations to the IRenderSceneBackend interface.
 */

#ifndef ENGINERENDERBRIDGE_HPP
#define ENGINERENDERBRIDGE_HPP

#include <array>
#include <unordered_map>

#include "world/RenderBridge.hpp"

namespace xaimassist::engine {
class Engine;
}

namespace xaimassist::app {

/**
 * @class EngineRenderBridge
 * @brief IRenderSceneBackend implementation that delegates to Engine
 *        and caches last-known state to skip redundant GPU updates.
 */
class EngineRenderBridge : public world::IRenderSceneBackend {
  public:
    explicit EngineRenderBridge(engine::Engine& engine);

    /// Create a sphere render object via the engine.
    RenderObjectId CreateSphere(const world::TransformComponent& transform,
                                const world::RenderComponent& render) override;

    /// Update an existing sphere's transform and appearance.
    bool UpdateSphere(RenderObjectId objectId,
                      const world::TransformComponent& transform,
                      const world::RenderComponent& render) override;

    /// Destroy a render object and remove its cached state.
    bool DestroyObject(RenderObjectId objectId) override;

  private:
    struct RenderStateCache {
        std::array<double, 3> position{0.0, 0.0, 0.0};
        std::array<double, 3> color{0.95, 0.35, 0.25};
        double opacity{1.0};
    };

    engine::Engine& m_engine;
    std::unordered_map<RenderObjectId, RenderStateCache> m_renderStateCache;
};
}  // namespace xaimassist::app

#endif  // ENGINERENDERBRIDGE_HPP
