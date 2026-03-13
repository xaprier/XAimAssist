/**
 * @file InputManager.hpp
 * @brief Captures mouse/keyboard input, applies sensitivity and
 *        dispatches normalised actions to registered handlers.
 */

#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <functional>

#include "input/InputActions.hpp"
#include "input/SensitivityModel.hpp"

class QEvent;
class QMouseEvent;
class QWidget;

namespace xaimassist::core {
class Logger;
}

namespace xaimassist::input {

/**
 * @class InputManager
 * @brief Qt event-filter that captures pointer motion and mouse clicks,
 *        transforms them via SensitivityModel, and invokes callbacks.
 */
class InputManager : public QObject {
  public:
    using LookHandler = std::function<void(const LookAction&)>;
    using FireHandler = std::function<void(const FireAction&)>;
    using EscapeHandler = std::function<void()>;
    using SceneClickHandler = std::function<bool()>;

    explicit InputManager(core::Logger& logger, QObject* parent = nullptr);
    ~InputManager() override;

    /// Begin capturing input from the given viewport widget.
    void AttachViewport(QWidget* viewportWidget);

    /// Stop capturing and release the viewport.
    void DetachViewport();

    /// Enable or disable mouse/keyboard capture.
    void SetCaptureEnabled(bool enabled);

    /// True if capture is currently active.
    bool IsCaptureEnabled() const noexcept;

    /// Validate the viewport widget.
    bool IsViewportValid() const noexcept;

    /// Enable or disable raw (unaccelerated) mouse input.
    void SetRawInputEnabled(bool enabled) noexcept;

    /// True if raw input mode is active.
    bool IsRawInputEnabled() const noexcept;

    /// Apply new sensitivity/DPI settings to the internal model.
    void SetSensitivitySettings(const SensitivitySettings& Settings);

    /// Return the current sensitivity settings.
    const SensitivitySettings& GetSensitivitySettings() const noexcept;

    /// Register the callback invoked on camera look deltas.
    void SetLookHandler(LookHandler handler);

    /// Register the callback invoked on fire button state changes.
    void SetFireHandler(FireHandler handler);

    /// Register the callback invoked when Escape is pressed.
    void SetEscapeHandler(EscapeHandler handler);

    /// Register the callback for scene-click gating (pre-capture).
    void SetSceneClickHandler(SceneClickHandler handler);

  protected:
    /// Qt event filter override that intercepts mouse/keyboard events.
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    void _CenterCursorInViewport();
    void _HandleMouseMove(const QMouseEvent& event);
    void _HandleMouseButton(bool pressed, int button);
    bool _ValidateViewport(const char* operation);

    core::Logger& m_logger;

    QPointer<QWidget> m_viewportWidget;
    SensitivityModel m_sensitivityModel;

    LookHandler m_lookHandler;
    FireHandler m_fireHandler;
    EscapeHandler m_escapeHandler;
    SceneClickHandler m_sceneClickHandler;

    bool m_captureEnabled{false};
    bool m_rawInputEnabled{true};
    bool m_recentering{false};

    bool m_hasLastPointerPosition{false};
    QPointF m_lastPointerGlobalPosition;
};
}  // namespace xaimassist::input

#endif  // INPUTMANAGER_HPP
