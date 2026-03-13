/**
 * @file InputActions.hpp
 * @brief Normalised input action structs consumed by gameplay.
 */

#ifndef INPUTACTIONS_HPP
#define INPUTACTIONS_HPP

namespace xaimassist::input {

/// Camera look delta produced after sensitivity transform.
struct LookAction {
    double yawDeltaDegrees{0.0};
    double pitchDeltaDegrees{0.0};
};

/// Primary fire button state.
struct FireAction {
    bool pressed{false};
};
}  // namespace xaimassist::input

#endif  // INPUTACTIONS_HPP
