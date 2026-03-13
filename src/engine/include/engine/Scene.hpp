/**
 * @file Scene.hpp
 * @brief Scene graph that manages VTK actors and environment visuals.
 */

#ifndef SCENE_HPP
#define SCENE_HPP

#include <vtkSmartPointer.h>

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

class vtkActor;
class vtkRenderer;

namespace xaimassist::engine {

/**
 * @class Scene
 * @brief Tracks spawned VTK actors and static environment geometry.
 *
 * Provides id-based CRUD operations over actors added to the renderer,
 * as well as background colour and environment decoration management.
 */
class Scene {
  public:
    using SceneObjectId = std::uint64_t;

    /// Assign the renderer that will host actors added to this scene.
    void SetRenderer(vtkSmartPointer<vtkRenderer> Renderer);

    /// Update the environment background colour and rebuild decorations.
    void SetEnvironmentBackgroundColor(const std::array<double, 3>& color);

    /// Insert an actor and return its stable id.
    SceneObjectId AddActor(vtkSmartPointer<vtkActor> actor);

    /// Move an actor to a new position.
    bool SetActorPosition(SceneObjectId objectId,
                          const std::array<double, 3>& position);
    /// Change an actor's diffuse colour.
    bool SetActorColor(SceneObjectId objectId,
                       const std::array<double, 3>& color);

    /// Set an actor's material opacity.
    bool SetActorOpacity(SceneObjectId objectId, double opacity);

    /// Remove and release an actor by id.
    bool RemoveActor(SceneObjectId objectId);

    /// Remove all user actors (environment stays).
    void Clear();

  private:
    static bool _ColorsEqual(const std::array<double, 3>& left,
                             const std::array<double, 3>& right) noexcept;
    static std::array<double, 3>
    _ClampColor(const std::array<double, 3>& color) noexcept;

    void _RebuildEnvironment();
    void _AddEnvironmentActor(vtkSmartPointer<vtkActor> actor);
    void _ClearEnvironmentActors();
    void _ApplyEnvironmentBackground();

    vtkSmartPointer<vtkRenderer> m_renderer;
    std::unordered_map<SceneObjectId, vtkSmartPointer<vtkActor>> m_actors;
    std::vector<vtkSmartPointer<vtkActor>> m_environmentActors;
    std::array<double, 3> m_environmentBackgroundColor{0.05, 0.08, 0.16};
    SceneObjectId m_nextObjectId{1};
};
}  // namespace xaimassist::engine

#endif  // SCENE_HPP
