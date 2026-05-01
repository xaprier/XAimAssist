/// @file Engine.cpp
#include "engine/Engine.hpp"

#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

#include <QWidget>
#include <algorithm>

#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "core/Logger.hpp"

namespace xaimassist::engine {
Engine::Engine(core::EventBus& eventBus, core::Logger& logger)
    : m_eventBus(eventBus), m_logger(logger), m_cameraBackend(logger) {}

Engine::~Engine() { Shutdown(); }

QSurfaceFormat Engine::RecommendedSurfaceFormat() {
    QSurfaceFormat fmt = QVTKOpenGLNativeWidget::defaultFormat();

    // OpenGL 4.1 Core is the highest version available on macOS and is broadly
    // supported on NVIDIA/AMD/Intel across all target platforms.
    fmt.setVersion(4, 1);
    fmt.setProfile(QSurfaceFormat::CoreProfile);

    // swap_interval=0: immediate present — no vertical sync, no added latency.
    // Competitive aim trainers always disable VSync for minimum input-to-pixel
    // delay. Users who want VSync can enable it via the driver control panel.
    fmt.setSwapInterval(0);

    // SwapBehavior::DoubleBuffer: one back buffer only.
    // TripleBuffer adds ~1 frame of latency on some GL implementations.
    fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

    // Depth/stencil: 24-bit depth is sufficient for the scene distances used
    // in an aim trainer; 8-bit stencil reserved for future effects.
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);

    // No MSAA at the surface level — antialiasing (if desired) is better
    // handled in a post-process pass that doesn't bloat the default framebuffer.
    fmt.setSamples(0);

    return fmt;
}

QWidget* Engine::CreateViewport(QWidget* parent) {
    auto* viewportWidget = new QVTKOpenGLNativeWidget(parent);
    if (!_InitializeViewport(*viewportWidget)) {
        delete viewportWidget;
        return nullptr;
    }

    return viewportWidget;
}

void Engine::Shutdown() {
    if (!m_initialized) {
        return;
    }

    _UnsubscribeFromCoreEvents();
    m_scene.Clear();
    m_scene.SetRenderer(nullptr);
    m_cameraBackend.SetRenderer(nullptr);
    m_renderContext.Shutdown();

    m_initialized = false;
    m_logger.Info("engine", "Engine Shutdown complete");
}

Engine::SceneObjectId Engine::SpawnSphere(const SphereSpawnRequest& request) {
    if (!m_initialized) {
        return 0;
    }

    auto sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetRadius(request.radius);
    sphereSource->SetThetaResolution(request.thetaResolution);
    sphereSource->SetPhiResolution(request.phiResolution);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphereSource->GetOutputPort());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->SetPosition(request.position[0], request.position[1],
                       request.position[2]);
    actor->GetProperty()->SetOpacity(std::clamp(request.opacity, 0.0, 1.0));
    actor->GetProperty()->SetColor(request.color[0], request.color[1],
                                   request.color[2]);

    return m_scene.AddActor(actor);
}

bool Engine::SetObjectPosition(SceneObjectId objectId,
                               const std::array<double, 3>& position) {
    return m_scene.SetActorPosition(objectId, position);
}

bool Engine::SetObjectColor(SceneObjectId objectId,
                            const std::array<double, 3>& color) {
    return m_scene.SetActorColor(objectId, color);
}

bool Engine::SetObjectOpacity(SceneObjectId objectId, double opacity) {
    return m_scene.SetActorOpacity(objectId, opacity);
}

bool Engine::DespawnObject(SceneObjectId objectId) {
    return m_scene.RemoveActor(objectId);
}

void Engine::ClearScene() { m_scene.Clear(); }

void Engine::SetBackgroundColor(const std::array<double, 3>& color) {
    m_scene.SetEnvironmentBackgroundColor(color);
}

void Engine::SetCameraPose(const CameraPose& pose) {
    m_cameraBackend.SetPose(pose);
}

Engine::CameraPose Engine::GetCameraPose() const noexcept {
    return m_cameraBackend.Pose();
}

void Engine::ApplyCameraLookDelta(double yawDeltaDegrees,
                                  double pitchDeltaDegrees) {
    m_cameraBackend.ApplyLookDelta(yawDeltaDegrees, pitchDeltaDegrees);
}

void Engine::SetCameraFieldOfView(double fovDegrees) {
    m_cameraBackend.SetFieldOfView(fovDegrees);
}

bool Engine::IsInitialized() const noexcept { return m_initialized; }

bool Engine::_InitializeViewport(QVTKOpenGLNativeWidget& viewportWidget) {
    if (m_initialized) {
        return true;
    }

    if (!m_renderContext.Initialize(viewportWidget)) {
        m_logger.Error("engine", "Failed to Initialize Render context");
        return false;
    }

    m_scene.SetRenderer(m_renderContext.Renderer());
    m_cameraBackend.SetRenderer(m_renderContext.Renderer());

    SetBackgroundColor({0.05, 0.08, 0.16});
    SetCameraPose(CameraPose{});

    m_initialized = true;
    _SubscribeToCoreEvents();

    m_logger.Info("engine", "Engine initialization complete");
    return true;
}

void Engine::_SubscribeToCoreEvents() {
    if (m_subscriptionId != 0) {
        return;
    }

    m_subscriptionId =
        m_eventBus.Subscribe([this](const core::events::CoreEvent& event) {
            _HandleCoreEvent(event);
        });
}

void Engine::_UnsubscribeFromCoreEvents() {
    if (m_subscriptionId == 0) {
        return;
    }

    m_eventBus.Unsubscribe(m_subscriptionId);
    m_subscriptionId = 0;
}

void Engine::_HandleCoreEvent(const core::events::CoreEvent& event) {
    if (const auto* frameTickEvent =
            std::get_if<core::events::FrameTickEvent>(&event)) {
        _OnFrameTick(*frameTickEvent);
    }
}

void Engine::_OnFrameTick(const core::events::FrameTickEvent& frameTickEvent) {
    if (!m_initialized || frameTickEvent.frameSeconds <= 0.0) {
        return;
    }

    m_renderContext.Render();
}
}  // namespace xaimassist::engine
