/// @file TargetFactory.cpp
#include "gameplay/TargetFactory.hpp"

#include <memory>

namespace xaimassist::gameplay {
std::unique_ptr<Target> TargetFactory::CreateTarget(
    std::uint64_t TargetId, std::uint64_t SessionId,
    SceneObjectId sceneObjectId, const TargetDefinition& Definition,
    std::chrono::steady_clock::time_point SpawnedAt) const {
    switch (Definition.type) {
        case TargetType::Static:
            return std::make_unique<StaticTarget>(TargetId, SessionId, sceneObjectId,
                                                  Definition, SpawnedAt);
        case TargetType::Moving:
            return std::make_unique<MovingTarget>(TargetId, SessionId, sceneObjectId,
                                                  Definition, SpawnedAt);
        case TargetType::Strafing:
            return std::make_unique<StrafingTarget>(TargetId, SessionId, sceneObjectId,
                                                    Definition, SpawnedAt);
        case TargetType::Blinking:
            return std::make_unique<BlinkingTarget>(TargetId, SessionId, sceneObjectId,
                                                    Definition, SpawnedAt);
    }

    return nullptr;
}
}  // namespace xaimassist::gameplay
