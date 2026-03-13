/// @file GameModeRegistry.cpp
#include "gameplay/GameModeRegistry.hpp"

#include <utility>

namespace xaimassist::gameplay {
bool GameModeRegistry::RegisterMode(const GameModeMetadata& Metadata,
                                    GameModeFactory factory) {
    if (Metadata.id.empty() || !factory) {
        return false;
    }

    if (m_registry.find(Metadata.id) != m_registry.end()) {
        return false;
    }

    m_registry.emplace(Metadata.id, RegistryEntry{Metadata, std::move(factory)});

    return true;
}

bool GameModeRegistry::ContainsMode(const std::string& modeId) const {
    return m_registry.find(modeId) != m_registry.end();
}

std::vector<GameModeMetadata> GameModeRegistry::AvailableModes() const {
    std::vector<GameModeMetadata> result;
    result.reserve(m_registry.size());

    for (const auto& [_, entry] : m_registry) {
        result.push_back(entry.Metadata);
    }

    return result;
}

std::unique_ptr<GameMode> GameModeRegistry::CreateMode(
    const std::string& modeId, core::EventBus& eventBus, core::Logger& logger,
    ISceneCommandSink& sceneCommandSink, TargetSystem& targetSystem,
    IGameplaySettingsProvider& settingsProvider) const {
    const auto iterator = m_registry.find(modeId);
    if (iterator == m_registry.end()) {
        return nullptr;
    }

    return iterator->second.factory(eventBus, logger, sceneCommandSink,
                                    targetSystem, settingsProvider);
}
}  // namespace xaimassist::gameplay
