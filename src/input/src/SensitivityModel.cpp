/// @file SensitivityModel.cpp
#include "input/SensitivityModel.hpp"

namespace {
constexpr double CENTIMETERS_PER_INCH = 2.54;  // physical conversion factor
}

namespace xaimassist::input {
void SensitivityModel::SetSettings(const SensitivitySettings& Settings) {
    m_settings = Settings;
}

const SensitivitySettings& SensitivityModel::Settings() const noexcept {
    return m_settings;
}

double SensitivityModel::DegreesPerCountYaw() const noexcept {
    return DegreesPerCount(m_settings.cmPer360, m_settings.dpi) *
           m_settings.sensitivityScale;
}

double SensitivityModel::DegreesPerCountPitch() const noexcept {
    return DegreesPerCount(m_settings.cmPer360, m_settings.dpi) *
           m_settings.sensitivityScale;
}

std::pair<double, double>
SensitivityModel::ComputeLookDeltaDegrees(double deltaXCounts,
                                          double deltaYCounts) const noexcept {
    const double yawDelta = deltaXCounts * DegreesPerCountYaw();

    double pitchDelta = -deltaYCounts * DegreesPerCountPitch();
    if (m_settings.invertY) {
        pitchDelta = -pitchDelta;
    }

    return {yawDelta, pitchDelta};
}

double SensitivityModel::CountsPer360(double CmPer360, double Dpi) noexcept {
    if (CmPer360 <= 0.0 || Dpi <= 0.0) {
        return 0.0;
    }

    return (CmPer360 / CENTIMETERS_PER_INCH) * Dpi;
}

double SensitivityModel::DegreesPerCount(double CmPer360, double Dpi) noexcept {
    const double counts = CountsPer360(CmPer360, Dpi);
    if (counts <= 0.0) {
        return 0.0;
    }

    return 360.0 / counts;
}
}  // namespace xaimassist::input
