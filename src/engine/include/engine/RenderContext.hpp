/**
 * @file RenderContext.hpp
 * @brief Owns the VTK render window and renderer attached to a Qt widget.
 */

#ifndef RENDERCONTEXT_HPP
#define RENDERCONTEXT_HPP

#include <vtkSmartPointer.h>

class QVTKOpenGLNativeWidget;
class vtkGenericOpenGLRenderWindow;
class vtkRenderer;

namespace xaimassist::engine {

/**
 * @class RenderContext
 * @brief Manages the lifetime of the VTK renderer and its OpenGL render window.
 */
class RenderContext {
  public:
    /// Create and attach VTK pipeline to the given Qt/VTK widget.
    bool Initialize(QVTKOpenGLNativeWidget& viewportWidget);

    /// Tear down the VTK pipeline.
    void Shutdown();

    /// Raw pointer to the VTK renderer (nullptr before initialisation).
    vtkSmartPointer<vtkRenderer> Renderer() const noexcept;

    /// True if the VTK pipeline has been successfully created.
    bool IsInitialized() const noexcept;

    /// Issue a render pass.
    void Render();

  private:
    QVTKOpenGLNativeWidget* m_viewportWidget{nullptr};
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
};
}  // namespace xaimassist::engine

#endif  // RENDERCONTEXT_HPP
