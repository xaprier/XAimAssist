/**
 * @file WorldRenderSync.hpp
 * @brief Synchronises dirty world entities with the render backend.
 */

#ifndef WORLDRENDERSYNC_HPP
#define WORLDRENDERSYNC_HPP

#include <unordered_map>

#include "world/RenderBridge.hpp"
#include "world/World.hpp"

namespace xaimassist::world {

/**
 * @class WorldRenderSync
 * @brief Incrementally pushes World component changes to an
 * IRenderSceneBackend.
 *
 * On each Sync() call it processes the dirty set, creates/updates/removes
 * render objects, and advances the topology revision watermark.
 */
class WorldRenderSync {
  public:
    WorldRenderSync(World& world, IRenderSceneBackend& renderBackend);

    /// Push all pending changes to the render backend.
    void Sync();

    /// Drop all bindings; next Sync() will recreate everything.
    void Reset();

  private:
    World& m_world;
    IRenderSceneBackend& m_renderBackend;
    std::unordered_map<EntityId, IRenderSceneBackend::RenderObjectId>
        m_renderBindings;
    std::uint64_t m_lastTopologyRevision{0};
};
}  // namespace xaimassist::world

#endif  // WORLDRENDERSYNC_HPP
