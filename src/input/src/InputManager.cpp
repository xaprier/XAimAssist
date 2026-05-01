/// @file InputManager.cpp
#include "input/InputManager.hpp"

#include <QCursor>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWidget>
#include <cmath>

#include "core/Logger.hpp"

namespace xaimassist::input {
InputManager::InputManager(core::Logger& logger, QObject* parent) : QObject(parent), m_logger(logger) {}

InputManager::~InputManager() { DetachViewport(); }

void InputManager::AttachViewport(QWidget* viewportWidget) {
    if (viewportWidget == nullptr) {
        m_logger.Warning("input", "Attempted to attach null viewport");
        return;
    }

    if (m_viewportWidget == viewportWidget) {
        return;
    }

    DetachViewport();

    m_viewportWidget = viewportWidget;

    if (m_viewportWidget.isNull()) {
        m_logger.Error("input", "Widget became null immediately after assignment");
        return;
    }

    m_viewportWidget->installEventFilter(this);
    m_viewportWidget->setFocusPolicy(Qt::StrongFocus);
    m_viewportWidget->setMouseTracking(true);
}

void InputManager::DetachViewport() {
    if (m_viewportWidget.isNull()) {
        return;
    }

    SetCaptureEnabled(false);

    // Safe to call even if widget was deleted
    if (!m_viewportWidget.isNull()) {
        m_viewportWidget->removeEventFilter(this);
    }

    m_viewportWidget.clear();
    m_hasLastPointerPosition = false;
}

void InputManager::SetCaptureEnabled(bool enabled) {
    if (m_captureEnabled == enabled) {
        return;
    }

    m_captureEnabled = enabled;

    if (!_ValidateViewport("SetCaptureEnabled")) {
        m_captureEnabled = false;
        return;
    }

    if (m_captureEnabled) {
        m_viewportWidget->setCursor(Qt::BlankCursor);
        m_viewportWidget->setFocus();
        m_viewportWidget->activateWindow();
        m_viewportWidget->grabMouse();
        m_hasLastPointerPosition = false;
        _CenterCursorInViewport();
        return;
    }

    m_viewportWidget->releaseMouse();
    m_viewportWidget->unsetCursor();
    m_recentering = false;
    m_hasLastPointerPosition = false;
}

bool InputManager::IsCaptureEnabled() const noexcept {
    return m_captureEnabled && IsViewportValid();
}

bool InputManager::IsViewportValid() const noexcept {
    return !m_viewportWidget.isNull();
}

void InputManager::SetRawInputEnabled(bool enabled) noexcept {
    m_rawInputEnabled = enabled;
}

bool InputManager::IsRawInputEnabled() const noexcept {
    return m_rawInputEnabled;
}

void InputManager::SetSensitivitySettings(const SensitivitySettings& Settings) {
    m_sensitivityModel.SetSettings(Settings);
}

const SensitivitySettings&
InputManager::GetSensitivitySettings() const noexcept {
    return m_sensitivityModel.Settings();
}

void InputManager::SetLookHandler(LookHandler handler) {
    m_lookHandler = std::move(handler);
}

void InputManager::SetFireHandler(FireHandler handler) {
    m_fireHandler = std::move(handler);
}

void InputManager::SetEscapeHandler(EscapeHandler handler) {
    m_escapeHandler = std::move(handler);
}

void InputManager::SetSceneClickHandler(SceneClickHandler handler) {
    m_sceneClickHandler = std::move(handler);
}

bool InputManager::eventFilter(QObject* watched, QEvent* event) {
    if (m_viewportWidget.isNull()) {
        m_logger.Warning("input",
                         "Event filter called but viewport is null - detaching");
        DetachViewport();
        return QObject::eventFilter(watched, event);
    }

    if (watched != m_viewportWidget.data()) {
        return QObject::eventFilter(watched, event);
    }

    switch (event->type()) {
        case QEvent::MouseButtonPress: {
            const auto* mouseEvent = static_cast<QMouseEvent*>(event);

            if (!m_captureEnabled) {
                if (mouseEvent->button() == Qt::LeftButton && m_sceneClickHandler) {
                    if (m_sceneClickHandler()) {
                        return true;
                    }
                }

                return QObject::eventFilter(watched, event);
            }

            _HandleMouseButton(true, mouseEvent->button());
            return true;
        }
        case QEvent::MouseButtonRelease: {
            if (!m_captureEnabled) {
                return QObject::eventFilter(watched, event);
            }

            _HandleMouseButton(false, static_cast<QMouseEvent*>(event)->button());
            return true;
        }
        case QEvent::MouseMove:
            if (!m_captureEnabled) {
                return QObject::eventFilter(watched, event);
            }

            _HandleMouseMove(*static_cast<QMouseEvent*>(event));
            return true;
        case QEvent::KeyPress: {
            const auto* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                if (m_escapeHandler) {
                    m_escapeHandler();
                } else if (m_captureEnabled) {
                    SetCaptureEnabled(false);
                }

                return true;
            }

            if (!m_captureEnabled) {
                return QObject::eventFilter(watched, event);
            }

            break;
        }
        default:
            if (!m_captureEnabled) {
                return QObject::eventFilter(watched, event);
            }

            break;
    }

    return QObject::eventFilter(watched, event);
}

void InputManager::_CenterCursorInViewport() {
    if (!_ValidateViewport("_CenterCursorInViewport")) {
        return;
    }

    const QPoint localCenter = m_viewportWidget->rect().center();
    const QPoint globalCenter = m_viewportWidget->mapToGlobal(localCenter);
    m_recentering = true;
    QCursor::setPos(globalCenter);
}

void InputManager::_HandleMouseMove(const QMouseEvent& event) {
    if (!_ValidateViewport("_HandleMouseMove")) {
        return;
    }

    if (m_recentering) {
        m_recentering = false;
        return;
    }

    double deltaX = 0.0;
    double deltaY = 0.0;

    if (m_rawInputEnabled) {
        const QPointF localCenter = m_viewportWidget->rect().center();
        const QPointF mouseLocalPosition = event.position();
        deltaX = mouseLocalPosition.x() - localCenter.x();
        deltaY = mouseLocalPosition.y() - localCenter.y();

        _CenterCursorInViewport();
    } else {
        const QPointF mouseGlobalPosition = event.globalPosition();
        if (!m_hasLastPointerPosition) {
            m_lastPointerGlobalPosition = mouseGlobalPosition;
            m_hasLastPointerPosition = true;
            return;
        }

        deltaX = mouseGlobalPosition.x() - m_lastPointerGlobalPosition.x();
        deltaY = mouseGlobalPosition.y() - m_lastPointerGlobalPosition.y();
        m_lastPointerGlobalPosition = mouseGlobalPosition;
    }

    if (std::abs(deltaX) < 0.0001 && std::abs(deltaY) < 0.0001) {
        return;
    }

    if (!m_lookHandler) {
        return;
    }

    const auto [yawDeltaDegrees, pitchDeltaDegrees] =
        m_sensitivityModel.ComputeLookDeltaDegrees(deltaX, deltaY);

    m_lookHandler(LookAction{yawDeltaDegrees, pitchDeltaDegrees});
}

void InputManager::_HandleMouseButton(bool pressed, int button) {
    if (button != Qt::LeftButton) {
        return;
    }

    if (!m_fireHandler) {
        return;
    }

    m_fireHandler(FireAction{pressed});
}

bool InputManager::_ValidateViewport(const char* operation) {
    if (m_viewportWidget.isNull()) {
        m_logger.Warning("input",
                         std::string("Operation '") + operation +
                             "' called but viewport is null or deleted");
        return false;
    }

    return true;
}
}  // namespace xaimassist::input
