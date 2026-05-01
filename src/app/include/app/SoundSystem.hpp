/**
 * @file SoundSystem.hpp
 * @brief Plays hit and miss audio feedback via Qt Multimedia.
 *
 * Subscribes to ShotHitEvent and ShotMissEvent on the EventBus and plays
 * the configured sound variant. Volume and enabled state can be updated
 * at runtime via ApplySettings().
 */

#ifndef SOUNDSYSTEM_HPP
#define SOUNDSYSTEM_HPP

#include <QSoundEffect>
#include <QUrl>
#include <array>
#include <cstdint>

#include "core/CoreEvents.hpp"
#include "core/EventBus.hpp"
#include "persistence/AppSettings.hpp"

namespace xaimassist::app {

class SoundSystem {
  public:
    explicit SoundSystem(core::EventBus& eventBus);
    ~SoundSystem();

    /// Apply (or re-apply) sound preferences at runtime.
    void ApplySettings(const persistence::SoundPreferences& prefs);

  private:
    void _OnShotHit(const core::events::ShotHitEvent&);
    void _OnShotMiss(const core::events::ShotMissEvent&);

    static QUrl _HitSoundUrl(persistence::HitSoundVariant variant);
    static QUrl _MissSoundUrl(persistence::MissSoundVariant variant);

    core::EventBus& m_eventBus;
    std::uint64_t m_hitSubId{0};
    std::uint64_t m_missSubId{0};

    persistence::SoundPreferences m_prefs;

    QSoundEffect m_hitEffect;
    QSoundEffect m_missEffect;
};

}  // namespace xaimassist::app

#endif  // SOUNDSYSTEM_HPP
