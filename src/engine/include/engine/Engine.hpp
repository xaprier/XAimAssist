/**
 * @file Engine.hpp
 * @brief Top-level rendering engine facade.
 *
 * Orchestrates RenderContext, Scene and CameraBackend while
 * exposing a high-level API that other modules consume.
 */

#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <QSurfaceFormat>
#include <array>
#include <cstdint>

#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "engine/CameraBackend.hpp"
#include "engine/RenderContext.hpp"
#include "engine/Scene.hpp"

namespace xaimassist::core {
class Logger;
}

class QVTKOpenGLNativeWidget;
class QWidget;

namespace xaimassist::engine {

/**
 * @class Engine
 * @brief High-level rendering facade for sphere spawning, camera control
 *        and per-frame render passes.
 */
class Engine {
  public:
    using SceneObjectId = Scene::SceneObjectId;
    using CameraPose = CameraBackend::CameraPose;

    /// Parameters for spawning a sphere actor in the scene.
    struct SphereSpawnRequest {
        double radius{0.5};
        std::array<double, 3> position{0.0, 0.0, 0.0};
        std::array<double, 3> color{0.95, 0.35, 0.25};
        double opacity{1.0};
        std::int32_t thetaResolution{48};
        std::int32_t phiResolution{48};
    };

    Engine(core::EventBus& eventBus, core::Logger& logger);
    ~Engine();

    /// Surface format suitable for VTK OpenGL rendering.
    static QSurfaceFormat RecommendedSurfaceFormat();

    /// Build and return the Qt widget hosting the VTK viewport.
    QWidget* CreateViewport(QWidget* parent);

    /// Tear down the engine and release all VTK resources.
    void Shutdown();

    /// Create a sphere actor from the given parameters and return its id.
    [[nodiscard]] SceneObjectId SpawnSphere(const SphereSpawnRequest& request);

    /// Move an existing scene object to a new position.
    [[nodiscard]] bool SetObjectPosition(SceneObjectId objectId,
                                         const std::array<double, 3>& position);

    /// Change the diffuse colour of a scene object.
    [[nodiscard]] bool SetObjectColor(SceneObjectId objectId,
                                      const std::array<double, 3>& color);

    /// Change the opacity of a scene object (0 = invisible, 1 = opaque).
    [[nodiscard]] bool SetObjectOpacity(SceneObjectId objectId, double opacity);

    /// Remove a scene object by id.
    [[nodiscard]] bool DespawnObject(SceneObjectId objectId);

    /// Remove all spawned objects from the scene.
    void ClearScene();

    /// Set the renderer background colour (normalised RGB).
    void SetBackgroundColor(const std::array<double, 3>& color);

    /// Overwrite camera orientation and position.
    void SetCameraPose(const CameraPose& pose);

    /// Return a snapshot of the current camera pose.
    CameraPose GetCameraPose() const noexcept;

    /// Incrementally rotate the camera by yaw/pitch deltas.
    void ApplyCameraLookDelta(double yawDeltaDegrees, double pitchDeltaDegrees);

    /// Set the camera vertical field-of-view.
    void SetCameraFieldOfView(double fovDegrees);

    /// True once the viewport and VTK pipeline are ready.
    bool IsInitialized() const noexcept;

  private:
    bool _InitializeViewport(QVTKOpenGLNativeWidget& viewportWidget);

    void _SubscribeToCoreEvents();
    void _UnsubscribeFromCoreEvents();
    void _HandleCoreEvent(const core::events::CoreEvent& event);
    void _OnFrameTick(const core::events::FrameTickEvent& frameTickEvent);

    core::EventBus& m_eventBus;
    core::Logger& m_logger;
    RenderContext m_renderContext;
    Scene m_scene;
    CameraBackend m_cameraBackend;
    core::EventBus::SubscriptionId m_subscriptionId{0};
    bool m_initialized{false};
};
}  // namespace xaimassist::engine

#endif  // ENGINE_HPP
