/// @file RuntimeLoop.cpp
#include "app/RuntimeLoop.hpp"

#include <QObject>

#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "core/FrameClock.hpp"

namespace {
constexpr int RUNTIME_LOOP_INTERVAL_MS = 4;
}

namespace xaimassist::app {
RuntimeLoop::RuntimeLoop(core::FrameClock& frameClock, core::EventBus& eventBus)
    : m_frameClock(frameClock), m_eventBus(eventBus) {
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(RUNTIME_LOOP_INTERVAL_MS);

    QObject::connect(&m_timer, &QTimer::timeout, [&]() { _OnTimeout(); });
}

void RuntimeLoop::Start() {
    if (m_running) {
        return;
    }

    m_frameClock.Reset();
    m_timer.start();
    m_running = true;
}

void RuntimeLoop::Stop() {
    if (!m_running) {
        return;
    }

    m_timer.stop();
    m_running = false;
}

void RuntimeLoop::_OnTimeout() {
    const auto tickResult = m_frameClock.Tick();
    m_eventBus.Publish(core::events::FrameTickEvent{
        tickResult.frameIndex, tickResult.frameSeconds, tickResult.fixedSteps,
        m_frameClock.FixedStepSeconds()});
}
}  // namespace xaimassist::app
