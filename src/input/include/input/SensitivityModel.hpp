/**
 * @file SensitivityModel.hpp
 * @brief Physical-first mouse sensitivity conversion (cm/360 + DPI).
 */

#ifndef SENSITIVITYMODEL_HPP
#define SENSITIVITYMODEL_HPP

#include <utility>

namespace xaimassist::input {

/// User-facing sensitivity preferences.
struct SensitivitySettings {
    double cmPer360{34.0};         ///< Physical distance for a full 360 turn.
    double dpi{800.0};             ///< Mouse hardware DPI.
    double sensitivityScale{1.0};  ///< Additional multiplier.
    double yawMultiplier{1.0};
    double pitchMultiplier{1.0};
    bool invertY{false};
};

/**
 * @class SensitivityModel
 * @brief Converts raw mouse counts to degrees using cm/360 + DPI.
 */
class SensitivityModel {
  public:
    /// Apply new sensitivity preferences.
    void SetSettings(const SensitivitySettings& Settings);

    /// Return the current settings.
    const SensitivitySettings& Settings() const noexcept;

    /// Degrees of yaw rotation per raw mouse count.
    double DegreesPerCountYaw() const noexcept;

    /// Degrees of pitch rotation per raw mouse count.
    double DegreesPerCountPitch() const noexcept;

    /**
     * @brief Convert raw mouse delta counts into yaw/pitch degrees.
     * @return (yawDegrees, pitchDegrees) pair.
     */
    std::pair<double, double>
    ComputeLookDeltaDegrees(double deltaXCounts,
                            double deltaYCounts) const noexcept;

    /// Raw counts needed for one full 360-degree turn.
    static double CountsPer360(double CmPer360, double Dpi) noexcept;

    /// Degrees per single mouse count.
    static double DegreesPerCount(double CmPer360, double Dpi) noexcept;

  private:
    SensitivitySettings m_settings;
};
}  // namespace xaimassist::input

#endif  // SENSITIVITYMODEL_HPP
