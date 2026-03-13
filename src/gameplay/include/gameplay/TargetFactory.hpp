/**
 * @file TargetFactory.hpp
 * @brief Creates concrete Target subclasses from a TargetDefinition.
 */

#ifndef TARGETFACTORY_HPP
#define TARGETFACTORY_HPP

#include <chrono>
#include <memory>

#include "gameplay/targets/Target.hpp"

namespace xaimassist::gameplay {

/**
 * @class TargetFactory
 * @brief Instantiates the correct Target subtype (Static, Moving, etc.)
 *        based on TargetDefinition::type.
 */
class TargetFactory {
  public:
    /// Create a concrete target subclass matching Definition.type.
    std::unique_ptr<Target> CreateTarget(std::uint64_t TargetId, std::uint64_t SessionId,
                                         SceneObjectId sceneObjectId, const TargetDefinition& Definition,
                                         std::chrono::steady_clock::time_point SpawnedAt) const;
};
}  // namespace xaimassist::gameplay

#endif  // TARGETFACTORY_HPP
