/// @file SoundSystem.cpp
#include "app/SoundSystem.hpp"

#include <QString>
#include <variant>

namespace xaimassist::app {

SoundSystem::SoundSystem(core::EventBus& eventBus) : m_eventBus(eventBus) {
    m_hitSubId = m_eventBus.Subscribe(
        [this](const core::events::CoreEvent& event) {
            if (const auto* e =
                    std::get_if<core::events::ShotHitEvent>(&event)) {
                _OnShotHit(*e);
            }
        });

    m_missSubId = m_eventBus.Subscribe(
        [this](const core::events::CoreEvent& event) {
            if (const auto* e =
                    std::get_if<core::events::ShotMissEvent>(&event)) {
                _OnShotMiss(*e);
            }
        });

    ApplySettings(persistence::SoundPreferences{});
}

SoundSystem::~SoundSystem() {
    m_eventBus.Unsubscribe(m_hitSubId);
    m_eventBus.Unsubscribe(m_missSubId);
}

void SoundSystem::ApplySettings(const persistence::SoundPreferences& prefs) {
    m_prefs = prefs;

    const QUrl hitUrl = _HitSoundUrl(prefs.hitSound);
    if (m_hitEffect.source() != hitUrl) {
        m_hitEffect.setSource(hitUrl);
    }
    m_hitEffect.setVolume(static_cast<float>(prefs.volume));

    const QUrl missUrl = _MissSoundUrl(prefs.missSound);
    if (m_missEffect.source() != missUrl) {
        m_missEffect.setSource(missUrl);
    }
    m_missEffect.setVolume(static_cast<float>(prefs.volume));
}

void SoundSystem::_OnShotHit(const core::events::ShotHitEvent&) {
    if (!m_prefs.enabled) {
        return;
    }
    m_hitEffect.play();
}

void SoundSystem::_OnShotMiss(const core::events::ShotMissEvent&) {
    if (!m_prefs.enabled) {
        return;
    }
    m_missEffect.play();
}

QUrl SoundSystem::_HitSoundUrl(persistence::HitSoundVariant variant) {
    switch (variant) {
        case persistence::HitSoundVariant::Click:
            return QUrl(QStringLiteral("qrc:/xaimassist/ui/sounds/hit_click.wav"));
        case persistence::HitSoundVariant::Gunshot:
            return QUrl(QStringLiteral("qrc:/xaimassist/ui/sounds/hit_gunshot.wav"));
        case persistence::HitSoundVariant::Pop:
            return QUrl(QStringLiteral("qrc:/xaimassist/ui/sounds/hit_pop.wav"));
        case persistence::HitSoundVariant::Swish:
        default:
            return QUrl(QStringLiteral("qrc:/xaimassist/ui/sounds/hit_swish.wav"));
    }
}

QUrl SoundSystem::_MissSoundUrl(persistence::MissSoundVariant variant) {
    switch (variant) {
        case persistence::MissSoundVariant::Empty:
            return QUrl(QStringLiteral("qrc:/xaimassist/ui/sounds/miss_empty.wav"));
        case persistence::MissSoundVariant::Beep:
            return QUrl(QStringLiteral("qrc:/xaimassist/ui/sounds/miss_beep.wav"));
        case persistence::MissSoundVariant::Tap:
            return QUrl(QStringLiteral("qrc:/xaimassist/ui/sounds/miss_tap.wav"));
        case persistence::MissSoundVariant::MissPop:
            return QUrl(QStringLiteral("qrc:/xaimassist/ui/sounds/miss_pop.wav"));
        case persistence::MissSoundVariant::Oops:
            return QUrl(QStringLiteral("qrc:/xaimassist/ui/sounds/miss_oops.wav"));
        case persistence::MissSoundVariant::Ricochet:
        default:
            return QUrl(QStringLiteral("qrc:/xaimassist/ui/sounds/miss_ricochet.wav"));
    }
}

}  // namespace xaimassist::app
