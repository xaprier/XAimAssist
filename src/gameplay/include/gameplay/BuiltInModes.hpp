/**
 * @file BuiltInModes.hpp
 * @brief Registers all built-in game modes into the registry.
 */

#ifndef BUILTINMODES_HPP
#define BUILTINMODES_HPP

namespace xaimassist::gameplay {
class GameModeRegistry;

/// Populate @p registry with every shipped mode.
void RegisterBuiltInModes(GameModeRegistry& registry);
}  // namespace xaimassist::gameplay

#endif  // BUILTINMODES_HPP
