/// @file FrameClock.cpp
#include "core/FrameClock.hpp"

#include <algorithm>

namespace xaimassist::core {
FrameClock::FrameClock(double fixedStepSeconds, double maxFrameSeconds)
    : m_fixedStep(std::chrono::duration<double>(fixedStepSeconds)),
      m_maxFrame(std::chrono::duration<double>(maxFrameSeconds)) {}

void FrameClock::Reset() {
    m_initialized = false;
    m_accumulator = std::chrono::duration<double>(0.0);
    m_frameIndex = 0;
}

FrameClock::TickResult FrameClock::Tick() {
    const auto now = Clock::now();

    if (!m_initialized) {
        m_initialized = true;
        m_lastSample = now;
        return {};
    }

    auto frameDelta = now - m_lastSample;
    m_lastSample = now;

    if (frameDelta < Clock::duration::zero()) {
        frameDelta = Clock::duration::zero();
    }

    const auto frameSeconds = std::chrono::duration<double>(frameDelta);
    // Spiral-of-death guard: cap delta & limit fixed sub-ticks
    const auto clampedFrameSeconds = std::min(frameSeconds, m_maxFrame);

    m_accumulator += clampedFrameSeconds;

    std::uint32_t fixedSteps = 0;
    while (m_accumulator >= m_fixedStep && fixedSteps < MAX_STEPS_PER_TICK) {
        m_accumulator -= m_fixedStep;
        ++fixedSteps;
    }

    ++m_frameIndex;

    return TickResult{clampedFrameSeconds.count(), fixedSteps, m_frameIndex};
}

double FrameClock::FixedStepSeconds() const noexcept {
    return m_fixedStep.count();
}
}  // namespace xaimassist::core
