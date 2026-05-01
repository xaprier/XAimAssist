/// @file RenderContext.cpp
#include "engine/RenderContext.hpp"

#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>

namespace xaimassist::engine {
bool RenderContext::Initialize(QVTKOpenGLNativeWidget& viewportWidget) {
    if (m_renderer != nullptr && m_renderWindow != nullptr) {
        return true;
    }

    m_viewportWidget = &viewportWidget;
    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_renderer = vtkSmartPointer<vtkRenderer>::New();

    // Disable VSync: for a competitive aim trainer, latency matters more than
    // tear-free presentation. swap_interval=0 lets the GPU render as fast as
    // possible and is the standard setting in competitive FPS applications.
    m_renderWindow->SetSwapControl(0);

    // Enable double buffering (front + one back buffer). Triple-buffering
    // is not requested; driver-side triple-buffering is disabled via VSync=off.
    m_renderWindow->DoubleBufferOn();

    // Disable VTK-level multisampling; QSurfaceFormat also sets samples=0.
    // Both must be zero to avoid the driver silently enabling MSAA on one path.
    m_renderWindow->SetMultiSamples(0);

    m_renderWindow->AddRenderer(m_renderer);
    m_viewportWidget->setRenderWindow(m_renderWindow);

    return true;
}

void RenderContext::Shutdown() {
    if (m_renderWindow != nullptr && m_renderer != nullptr) {
        m_renderWindow->RemoveRenderer(m_renderer);
    }

    m_renderer = nullptr;
    m_renderWindow = nullptr;
    m_viewportWidget = nullptr;
}

vtkSmartPointer<vtkRenderer> RenderContext::Renderer() const noexcept {
    return m_renderer;
}

bool RenderContext::IsInitialized() const noexcept {
    return m_renderer != nullptr && m_renderWindow != nullptr;
}

void RenderContext::Render() {
    if (!IsInitialized()) {
        return;
    }

    m_renderWindow->Render();
}
}  // namespace xaimassist::engine
