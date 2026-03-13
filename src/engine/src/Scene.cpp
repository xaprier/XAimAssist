/// @file Scene.cpp
#include "engine/Scene.hpp"

#include <vtkActor.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRenderer.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr double COLOR_EPSILON = 0.000001;
constexpr std::array<double, 3> SCENE_MIN_BOUNDS = {-150.0, -150.0, -150.0};
constexpr std::array<double, 3> SCENE_MAX_BOUNDS = {150.0, 150.0, 150.0};

double _Clamp01(double value) { return std::clamp(value, 0.0, 1.0); }

bool _NearlyEqual(double left, double right) {
    return std::abs(left - right) <= COLOR_EPSILON;
}

std::array<double, 3> _Mix(const std::array<double, 3>& left,
                           const std::array<double, 3>& right, double factor) {
    const double clampedFactor = std::clamp(factor, 0.0, 1.0);
    return {
        left[0] + (right[0] - left[0]) * clampedFactor,
        left[1] + (right[1] - left[1]) * clampedFactor,
        left[2] + (right[2] - left[2]) * clampedFactor,
    };
}

double _Luminance(const std::array<double, 3>& color) {
    return 0.2126 * color[0] + 0.7152 * color[1] + 0.0722 * color[2];
}

std::array<double, 3>
referenceColorForBackground(const std::array<double, 3>& background) {
    if (_Luminance(background) >= 0.55) {
        return {0.08, 0.1, 0.13};
    }

    return {0.9, 0.92, 0.96};
}

std::array<double, 3>
clampToSceneBounds(const std::array<double, 3>& position) {
    return {
        std::clamp(position[0], SCENE_MIN_BOUNDS[0], SCENE_MAX_BOUNDS[0]),
        std::clamp(position[1], SCENE_MIN_BOUNDS[1], SCENE_MAX_BOUNDS[1]),
        std::clamp(position[2], SCENE_MIN_BOUNDS[2], SCENE_MAX_BOUNDS[2]),
    };
}

vtkSmartPointer<vtkActor> createLineActor(const std::array<double, 3>& Start,
                                          const std::array<double, 3>& end,
                                          const std::array<double, 3>& color,
                                          double lineWidth, double opacity) {
    auto lineSource = vtkSmartPointer<vtkLineSource>::New();
    lineSource->SetPoint1(Start[0], Start[1], Start[2]);
    lineSource->SetPoint2(end[0], end[1], end[2]);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(lineSource->GetOutputPort());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->PickableOff();
    actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    actor->GetProperty()->SetLineWidth(lineWidth);
    actor->GetProperty()->SetOpacity(opacity);

    return actor;
}

vtkSmartPointer<vtkActor> createRingActor(const std::array<double, 3>& center,
                                          double radius,
                                          const std::array<double, 3>& normal,
                                          const std::array<double, 3>& color,
                                          double lineWidth, double opacity) {
    auto ringSource = vtkSmartPointer<vtkRegularPolygonSource>::New();
    ringSource->SetCenter(center[0], center[1], center[2]);
    ringSource->SetNormal(normal[0], normal[1], normal[2]);
    ringSource->SetRadius(radius);
    ringSource->SetNumberOfSides(96);
    ringSource->GeneratePolygonOff();

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(ringSource->GetOutputPort());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->PickableOff();
    actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    actor->GetProperty()->SetLineWidth(lineWidth);
    actor->GetProperty()->SetOpacity(opacity);

    return actor;
}
}  // namespace

namespace xaimassist::engine {
bool Scene::_ColorsEqual(const std::array<double, 3>& left,
                         const std::array<double, 3>& right) noexcept {
    return _NearlyEqual(left[0], right[0]) && _NearlyEqual(left[1], right[1]) &&
           _NearlyEqual(left[2], right[2]);
}

std::array<double, 3>
Scene::_ClampColor(const std::array<double, 3>& color) noexcept {
    return {_Clamp01(color[0]), _Clamp01(color[1]), _Clamp01(color[2])};
}

void Scene::SetRenderer(vtkSmartPointer<vtkRenderer> renderer) {
    if (m_renderer == renderer) {
        return;
    }

    if (m_renderer != nullptr) {
        for (const auto& actor : m_environmentActors) {
            m_renderer->RemoveActor(actor);
        }

        for (const auto& [_, actor] : m_actors) {
            m_renderer->RemoveActor(actor);
        }
    }

    m_renderer = renderer;

    if (m_renderer != nullptr) {
        _ApplyEnvironmentBackground();
        _RebuildEnvironment();

        for (const auto& [_, actor] : m_actors) {
            m_renderer->AddActor(actor);
        }
    }
}

void Scene::SetEnvironmentBackgroundColor(const std::array<double, 3>& color) {
    const auto clamped = _ClampColor(color);
    if (_ColorsEqual(m_environmentBackgroundColor, clamped)) {
        return;
    }

    m_environmentBackgroundColor = clamped;
    _ApplyEnvironmentBackground();
    _RebuildEnvironment();
}

Scene::SceneObjectId Scene::AddActor(vtkSmartPointer<vtkActor> actor) {
    if (actor == nullptr) {
        return 0;
    }

    std::array<double, 3> requestedPosition{0.0, 0.0, 0.0};
    actor->GetPosition(requestedPosition.data());
    const auto clampedPosition = clampToSceneBounds(requestedPosition);
    actor->SetPosition(clampedPosition[0], clampedPosition[1],
                       clampedPosition[2]);

    const SceneObjectId objectId = m_nextObjectId++;
    m_actors.emplace(objectId, actor);

    if (m_renderer != nullptr) {
        m_renderer->AddActor(actor);
    }

    return objectId;
}

bool Scene::SetActorPosition(SceneObjectId objectId,
                             const std::array<double, 3>& position) {
    const auto iterator = m_actors.find(objectId);
    if (iterator == m_actors.end()) {
        return false;
    }

    const auto clampedPosition = clampToSceneBounds(position);
    iterator->second->SetPosition(clampedPosition[0], clampedPosition[1],
                                  clampedPosition[2]);
    return true;
}

bool Scene::SetActorColor(SceneObjectId objectId,
                          const std::array<double, 3>& color) {
    const auto iterator = m_actors.find(objectId);
    if (iterator == m_actors.end()) {
        return false;
    }

    iterator->second->GetProperty()->SetColor(color[0], color[1], color[2]);
    return true;
}

bool Scene::SetActorOpacity(SceneObjectId objectId, double opacity) {
    const auto iterator = m_actors.find(objectId);
    if (iterator == m_actors.end()) {
        return false;
    }

    iterator->second->GetProperty()->SetOpacity(std::clamp(opacity, 0.0, 1.0));
    return true;
}

bool Scene::RemoveActor(SceneObjectId objectId) {
    const auto iterator = m_actors.find(objectId);
    if (iterator == m_actors.end()) {
        return false;
    }

    if (m_renderer != nullptr) {
        m_renderer->RemoveActor(iterator->second);
    }

    m_actors.erase(iterator);
    return true;
}

void Scene::Clear() {
    if (m_renderer != nullptr) {
        for (const auto& [_, actor] : m_actors) {
            m_renderer->RemoveActor(actor);
        }
    }

    m_actors.clear();
}

void Scene::_RebuildEnvironment() {
    _ClearEnvironmentActors();

    if (m_renderer == nullptr) {
        return;
    }

    const auto referenceColor =
        referenceColorForBackground(m_environmentBackgroundColor);

    const auto floorMinorColor =
        _Mix(m_environmentBackgroundColor, referenceColor, 0.20);
    const auto floorMajorColor =
        _Mix(m_environmentBackgroundColor, referenceColor, 0.36);
    const auto wallMinorColor =
        _Mix(m_environmentBackgroundColor, referenceColor, 0.24);
    const auto wallMajorColor =
        _Mix(m_environmentBackgroundColor, referenceColor, 0.42);

    constexpr double GRID_SPACING = 10.0;
    constexpr int GRID_MAJOR_STEP = 5;

    const double minX = SCENE_MIN_BOUNDS[0];
    const double minY = SCENE_MIN_BOUNDS[1];
    const double minZ = SCENE_MIN_BOUNDS[2];
    const double maxX = SCENE_MAX_BOUNDS[0];
    const double maxY = SCENE_MAX_BOUNDS[1];
    const double maxZ = SCENE_MAX_BOUNDS[2];

    const int xSteps = static_cast<int>((maxX - minX) / GRID_SPACING);
    const int ySteps = static_cast<int>((maxY - minY) / GRID_SPACING);
    const int zSteps = static_cast<int>((maxZ - minZ) / GRID_SPACING);

    auto addGridLine = [&](const std::array<double, 3>& Start,
                           const std::array<double, 3>& end, bool isMajor,
                           bool floorLikeSurface) {
        const auto& lineColor = floorLikeSurface
                                    ? (isMajor ? floorMajorColor : floorMinorColor)
                                    : (isMajor ? wallMajorColor : wallMinorColor);
        const double lineWidth = isMajor ? 1.8 : 1.0;
        const double opacity =
            floorLikeSurface ? (isMajor ? 0.62 : 0.36) : (isMajor ? 0.56 : 0.32);
        _AddEnvironmentActor(
            createLineActor(Start, end, lineColor, lineWidth, opacity));
    };

    for (int ix = 0; ix <= xSteps; ++ix) {
        const double x = minX + static_cast<double>(ix) * GRID_SPACING;
        const bool isMajorLine = (ix % GRID_MAJOR_STEP) == 0;

        addGridLine({x, minY, minZ}, {x, minY, maxZ}, isMajorLine, true);
        addGridLine({x, maxY, minZ}, {x, maxY, maxZ}, isMajorLine, true);
        addGridLine({x, minY, minZ}, {x, maxY, minZ}, isMajorLine, false);
        addGridLine({x, minY, maxZ}, {x, maxY, maxZ}, isMajorLine, false);
    }

    for (int iz = 0; iz <= zSteps; ++iz) {
        const double z = minZ + static_cast<double>(iz) * GRID_SPACING;
        const bool isMajorLine = (iz % GRID_MAJOR_STEP) == 0;

        addGridLine({minX, minY, z}, {maxX, minY, z}, isMajorLine, true);
        addGridLine({minX, maxY, z}, {maxX, maxY, z}, isMajorLine, true);
        addGridLine({minX, minY, z}, {minX, maxY, z}, isMajorLine, false);
        addGridLine({maxX, minY, z}, {maxX, maxY, z}, isMajorLine, false);
    }

    for (int iy = 0; iy <= ySteps; ++iy) {
        const double y = minY + static_cast<double>(iy) * GRID_SPACING;
        const bool isMajorLine = (iy % GRID_MAJOR_STEP) == 0;

        addGridLine({minX, y, minZ}, {maxX, y, minZ}, isMajorLine, false);
        addGridLine({minX, y, maxZ}, {maxX, y, maxZ}, isMajorLine, false);

        const auto& sideHorizontalColor =
            isMajorLine ? wallMajorColor : wallMinorColor;
        _AddEnvironmentActor(
            createLineActor({minX, y, minZ}, {minX, y, maxZ}, sideHorizontalColor,
                            isMajorLine ? 2.2 : 1.5, isMajorLine ? 0.74 : 0.54));
        _AddEnvironmentActor(
            createLineActor({maxX, y, minZ}, {maxX, y, maxZ}, sideHorizontalColor,
                            isMajorLine ? 2.2 : 1.5, isMajorLine ? 0.74 : 0.54));
    }

    _AddEnvironmentActor(createRingActor({0.0, minY + 3.0, 0.0}, 24.0,
                                         {0.0, 1.0, 0.0}, wallMajorColor, 1.4,
                                         0.35));
}

void Scene::_AddEnvironmentActor(vtkSmartPointer<vtkActor> actor) {
    if (actor == nullptr) {
        return;
    }

    m_environmentActors.push_back(actor);
    if (m_renderer != nullptr) {
        m_renderer->AddActor(actor);
    }
}

void Scene::_ClearEnvironmentActors() {
    if (m_renderer != nullptr) {
        for (const auto& actor : m_environmentActors) {
            m_renderer->RemoveActor(actor);
        }
    }

    m_environmentActors.clear();
}

void Scene::_ApplyEnvironmentBackground() {
    if (m_renderer == nullptr) {
        return;
    }

    const auto baseColor = _ClampColor(m_environmentBackgroundColor);
    const auto topColor = _Luminance(baseColor) >= 0.55
                              ? _Mix(baseColor, {0.0, 0.0, 0.0}, 0.16)
                              : _Mix(baseColor, {1.0, 1.0, 1.0}, 0.14);

    m_renderer->SetGradientBackground(true);
    m_renderer->SetBackground(baseColor[0], baseColor[1], baseColor[2]);
    m_renderer->SetBackground2(topColor[0], topColor[1], topColor[2]);
}
}  // namespace xaimassist::engine
