/**
 * @file CameraBackend.hpp
 * @brief FPS camera abstraction backed by VTK.
 */

#ifndef CAMERABACKEND_HPP
#define CAMERABACKEND_HPP

#include <vtkSmartPointer.h>

#include <array>

class vtkRenderer;

namespace xaimassist::core {
class Logger;
}

namespace xaimassist::engine {

/**
 * @class CameraBackend
 * @brief Maintains yaw/pitch/fov state and pushes it to a vtkRenderer camera.
 */
class CameraBackend {
  public:
    /// Full description of camera orientation and projection.
    struct CameraPose {
        std::array<double, 3> position{0.0, 0.0, 0.0};
        double yawDegrees{0.0};
        double pitchDegrees{0.0};
        double fovDegrees{60.0};
    };

    explicit CameraBackend(core::Logger& logger);

    /// Bind to a renderer whose camera will be driven.
    void SetRenderer(vtkSmartPointer<vtkRenderer> renderer);

    /// Overwrite the current pose and sync to VTK.
    void SetPose(const CameraPose& pose);

    /// Incrementally rotate; pitch is clamped to avoid gimbal flip.
    void ApplyLookDelta(double yawDeltaDegrees, double pitchDeltaDegrees);

    /// Set the vertical field of view in degrees.
    void SetFieldOfView(double fovDegrees);

    /// Return a copy of the current camera pose.
    CameraPose Pose() const noexcept;

  private:
    static double _ClampPitch(double pitchDegrees);
    static double _ClampFov(double fovDegrees);
    void _SyncToRenderer() const;

    vtkSmartPointer<vtkRenderer> m_renderer;
    CameraPose m_pose;
    core::Logger& m_logger;
};
}  // namespace xaimassist::engine

#endif  // CAMERABACKEND_HPP
