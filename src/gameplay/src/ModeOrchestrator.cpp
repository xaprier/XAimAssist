/// @file ModeOrchestrator.cpp
#include "gameplay/ModeOrchestrator.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "core/Logger.hpp"
#include "gameplay/SettingsProvider.hpp"
#include "gameplay/TargetSystem.hpp"

namespace xaimassist::gameplay {
ModeOrchestrator::ModeOrchestrator(core::EventBus& eventBus,
                                   core::Logger& logger,
                                   GameModeRegistry& registry,
                                   ISceneCommandSink& sceneCommandSink,
                                   TargetSystem& targetSystem,
                                   IGameplaySettingsProvider& settingsProvider)
    : m_eventBus(eventBus), m_logger(logger), m_registry(registry), m_sceneCommandSink(sceneCommandSink), m_targetSystem(targetSystem), m_settingsProvider(settingsProvider) {}

bool ModeOrchestrator::StartMode(
    const std::string& modeId, double durationOverrideSeconds,
    double distanceOverrideUnits,
    const std::unordered_map<std::string, double>& modeSettingOverrides) {
    if (HasActiveMode()) {
        StopMode();
    }

    auto mode =
        m_registry.CreateMode(modeId, m_eventBus, m_logger, m_sceneCommandSink,
                              m_targetSystem, m_settingsProvider);
    if (!mode) {
        m_logger.Error("gameplay", "Requested mode is not registered");
        return false;
    }

    m_activeSessionId = m_nextSessionId++;
    m_activeModeId = modeId;
    m_elapsedSeconds = 0.0;

    const double defaultDuration = mode->Metadata().defaultDurationSeconds;
    m_modeDurationSeconds =
        durationOverrideSeconds > 0.0 ? durationOverrideSeconds : defaultDuration;

    const double defaultDistance =
        std::max(0.1, mode->Metadata().defaultDistanceUnits);
    const double ConfiguredDistance =
        distanceOverrideUnits > 0.0 ? distanceOverrideUnits : defaultDistance;

    m_activeMode = std::move(mode);
    m_activeMode->SetSessionId(m_activeSessionId);
    m_activeMode->SetConfiguredDistance(std::max(0.1, ConfiguredDistance));
    m_activeMode->ApplyModeSettings(modeSettingOverrides);
    m_activeMode->OnStart();

    const GameplayRuntimeSettings runtimeSettings =
        m_settingsProvider.CurrentSettings();

    m_eventBus.Publish(core::events::SessionStartedEvent{
        m_activeSessionId, m_activeModeId, std::chrono::system_clock::now(),
        std::max(0.1, ConfiguredDistance),
        std::max(0.05, runtimeSettings.TargetRadius), modeSettingOverrides});

    {
        std::ostringstream debugStream;
        debugStream << "Mode Start details: SessionId=" << m_activeSessionId
                    << ", mode=" << m_activeModeId
                    << ", durationSeconds=" << m_modeDurationSeconds
                    << ", ConfiguredDistance=" << ConfiguredDistance
                    << ", settingCount="
                    << static_cast<unsigned long long>(modeSettingOverrides.size());
        m_logger.DebugIf("DEBUG_GAMEPLAY", "gameplay", debugStream.str());
    }

    m_logger.Info("gameplay", "Game mode started");
    return true;
}

void ModeOrchestrator::Update(double dtSeconds) {
    if (!HasActiveMode()) {
        return;
    }

    const double safeDelta = std::max(0.0, dtSeconds);
    m_elapsedSeconds += safeDelta;
    m_activeMode->OnUpdate(safeDelta);
    m_targetSystem.Update(safeDelta);

    if (m_modeDurationSeconds > 0.0 &&
        m_elapsedSeconds >= m_modeDurationSeconds) {
        StopMode();
    }
}

void ModeOrchestrator::StopMode(core::events::SessionStopReason stopReason) {
    if (!HasActiveMode()) {
        return;
    }

    m_activeMode->OnStop();
    m_targetSystem.ClearSessionTargets(m_activeSessionId);

    m_eventBus.Publish(core::events::SessionStoppedEvent{
        m_activeSessionId, m_activeModeId, std::chrono::system_clock::now(),
        m_elapsedSeconds, m_modeDurationSeconds, stopReason});

    {
        std::ostringstream debugStream;
        debugStream << "Mode Stop details: SessionId=" << m_activeSessionId
                    << ", mode=" << m_activeModeId
                    << ", ElapsedSeconds=" << m_elapsedSeconds
                    << ", configuredDurationSeconds=" << m_modeDurationSeconds;
        m_logger.DebugIf("DEBUG_GAMEPLAY", "gameplay", debugStream.str());
    }

    m_logger.Info("gameplay", "Game mode stopped");

    m_activeMode.reset();
    m_activeModeId.clear();
    m_activeSessionId = 0;
    m_elapsedSeconds = 0.0;
    m_modeDurationSeconds = 0.0;
}

bool ModeOrchestrator::HasActiveMode() const noexcept {
    return m_activeMode != nullptr;
}

const std::string& ModeOrchestrator::ActiveModeId() const noexcept {
    return m_activeModeId;
}

std::uint64_t ModeOrchestrator::ActiveSessionId() const noexcept {
    return m_activeSessionId;
}

double ModeOrchestrator::ElapsedSeconds() const noexcept {
    return m_elapsedSeconds;
}

double ModeOrchestrator::RemainingSeconds() const noexcept {
    if (!HasActiveMode()) {
        return 0.0;
    }

    if (m_modeDurationSeconds <= 0.0) {
        return 0.0;
    }

    return std::max(0.0, m_modeDurationSeconds - m_elapsedSeconds);
}
}  // namespace xaimassist::gameplay
