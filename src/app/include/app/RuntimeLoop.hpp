/**
 * @file RuntimeLoop.hpp
 * @brief QTimer-driven frame loop that ticks FrameClock and publishes
 * FrameTickEvent.
 */

#ifndef RUNTIMELOOP_HPP
#define RUNTIMELOOP_HPP

#include <QTimer>

namespace xaimassist::core {
class EventBus;
class FrameClock;
}  // namespace xaimassist::core

namespace xaimassist::app {

/**
 * @class RuntimeLoop
 * @brief Drives the core frame loop using a zero-interval QTimer.
 */
class RuntimeLoop {
  public:
    RuntimeLoop(core::FrameClock& frameClock, core::EventBus& eventBus);

    /// Start the zero-interval timer frame loop.
    void Start();

    /// Stop the frame loop.
    void Stop();

  private:
    void _OnTimeout();

    core::FrameClock& m_frameClock;
    core::EventBus& m_eventBus;
    QTimer m_timer;
    bool m_running{false};
};
}  // namespace xaimassist::app

#endif  // RUNTIMELOOP_HPP
