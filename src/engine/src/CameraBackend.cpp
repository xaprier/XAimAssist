/// @file CameraBackend.cpp
#include "engine/CameraBackend.hpp"

#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include <algorithm>
#include <cmath>

#include "core/Logger.hpp"

namespace {
constexpr double PI = 3.14159265358979323846;

double toRadians(double degrees) { return degrees * PI / 180.0; }
}  // namespace

namespace xaimassist::engine {

CameraBackend::CameraBackend(core::Logger& logger) : m_logger(logger) {
    m_logger.Debug("engine", "CameraBackend created");
}

void CameraBackend::SetRenderer(vtkSmartPointer<vtkRenderer> renderer) {
    if (m_renderer == renderer) {
        return;
    }

    m_renderer = renderer;

    if (m_renderer != nullptr) {
        _SyncToRenderer();
        m_logger.Debug("engine", "CameraBackend renderer set");
    } else {
        m_logger.Warning("engine", "CameraBackend renderer set to nullptr");
    }
}

void CameraBackend::SetPose(const CameraPose& pose) {
    m_pose = pose;
    m_pose.pitchDegrees = _ClampPitch(m_pose.pitchDegrees);
    m_pose.fovDegrees = _ClampFov(m_pose.fovDegrees);
    _SyncToRenderer();
}

void CameraBackend::ApplyLookDelta(double yawDeltaDegrees,
                                   double pitchDeltaDegrees) {
    m_pose.yawDegrees += yawDeltaDegrees;
    m_pose.pitchDegrees = _ClampPitch(m_pose.pitchDegrees + pitchDeltaDegrees);
    _SyncToRenderer();
}

void CameraBackend::SetFieldOfView(double fovDegrees) {
    m_pose.fovDegrees = _ClampFov(fovDegrees);
    _SyncToRenderer();
}

CameraBackend::CameraPose CameraBackend::Pose() const noexcept {
    return m_pose;
}

double CameraBackend::_ClampPitch(double pitchDegrees) {
    return std::clamp(pitchDegrees, -89.0, 89.0);
}

double CameraBackend::_ClampFov(double fovDegrees) {
    return std::clamp(fovDegrees, 30.0, 140.0);
}

void CameraBackend::_SyncToRenderer() const {
    if (m_renderer == nullptr) {
        m_logger.Debug("engine",
                       "Skipping sync: CameraBackend renderer is nullptr");
        return;
    }

    // what if m_renderer is not a valid pointer
    try {
        auto* camera = m_renderer->GetActiveCamera();
        if (camera == nullptr) {
            m_logger.Warning("engine", "CameraBackend renderer has no active camera");
            return;
        }

        const double yawRadians = toRadians(m_pose.yawDegrees);
        const double pitchRadians = toRadians(m_pose.pitchDegrees);

        const double directionX = std::cos(pitchRadians) * std::cos(yawRadians);
        const double directionY = std::sin(pitchRadians);
        const double directionZ = std::cos(pitchRadians) * std::sin(yawRadians);

        camera->SetPosition(m_pose.position[0], m_pose.position[1],
                            m_pose.position[2]);
        camera->SetFocalPoint(m_pose.position[0] + directionX,
                              m_pose.position[1] + directionY,
                              m_pose.position[2] + directionZ);
        camera->SetViewUp(0.0, 1.0, 0.0);
        camera->SetViewAngle(m_pose.fovDegrees);
        camera->Modified();

        m_renderer->ResetCameraClippingRange();
    } catch (const std::exception& ex) {
        m_logger.Error("engine",
                       std::string("Exception during camera sync: ") + ex.what());
    } catch (...) {
        m_logger.Error(
            "engine",
            "Unknown exception during camera sync - possible dangling pointer");
    }
}
}  // namespace xaimassist::engine
