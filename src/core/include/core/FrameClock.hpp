/**
 * @file FrameClock.hpp
 * @brief Fixed-timestep accumulator with variable frame delta.
 *
 * Provides deterministic fixed-step sub-ticks while exposing the
 * actual variable frame delta for rendering interpolation.
 */

#ifndef FRAMECLOCK_HPP
#define FRAMECLOCK_HPP

#include <chrono>
#include <cstdint>

namespace xaimassist::core {

/**
 * @class FrameClock
 * @brief Measures wall-clock deltas and produces fixed-step tick counts.
 */
class FrameClock {
  public:
    /// Result returned by each Tick() call.
    struct TickResult {
        double frameSeconds{0.0};     ///< Variable delta since last tick.
        std::uint32_t fixedSteps{0};  ///< Fixed sub-ticks accumulated this frame.
        std::uint64_t frameIndex{0};  ///< Monotonic frame counter.
    };

    /**
     * @param fixedStepSeconds Duration of one fixed sub-tick (default 1/120 s).
     * @param maxFrameSeconds  Cap to prevent spiral-of-death after long stalls.
     */
    explicit FrameClock(double fixedStepSeconds = 1.0 / 120.0,
                        double maxFrameSeconds = 0.25);

    /// Reset internal state; next Tick() starts fresh.
    void Reset();

    /// Sample wall clock, compute delta & fixed steps.
    [[nodiscard]] TickResult Tick();

    [[nodiscard]] double FixedStepSeconds() const noexcept;

  private:
    using Clock = std::chrono::steady_clock;

    static constexpr std::uint32_t MAX_STEPS_PER_TICK = 8;

    bool m_initialized{false};
    Clock::time_point m_lastSample;
    std::chrono::duration<double> m_accumulator{0.0};
    std::chrono::duration<double> m_fixedStep;
    std::chrono::duration<double> m_maxFrame;
    std::uint64_t m_frameIndex{0};
};
}  // namespace xaimassist::core

#endif  // FRAMECLOCK_HPP
