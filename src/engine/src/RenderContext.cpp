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
