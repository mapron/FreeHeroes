/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MusicBox.hpp"

#include "Logger.hpp"

#include <QSoundEffect>
#include <QMediaPlayer>
#include <QTimer>
#include <QPropertyAnimation>
#include <QMediaPlaylist>

namespace FreeHeroes::Sound {

struct MusicBox::Impl {
    /**
     * That weird array with effect cache due to QSoundEffect bug: when trying to delete/stop/destructor (same),
     * On Windows that will call sleep() in UI thread. That is awful, taht's why we don't ever stop effects, but
     * rather mute/unmute them.
     * empirical looks like 3 is just enough (2 probably would be fine too), to get some envelope when chaining
     * identical effects.
     * I chouse 3 to get some cool chorus effect.
     *
     * @todo: some strategy to cleanup this somehow... bad thing that this sleep stops music too.
     */
    struct EffectCache {
        QList<QSoundEffect*> m_effects;
        int                  m_lastIndex = 0;
    };
    QMap<std::string, EffectCache> m_effectsCache;

    QMediaPlayer   m_player;
    QMediaPlaylist m_playlist;
    int            m_musicVolumeLog        = 100;
    qreal          m_musicVolumeLinearBase = 1.;
    int            m_musicVolumeFinal      = 100;
    //qreal m_musicVolumeFade = 1.0;

    int   m_efectsVolumeLog        = 100;
    qreal m_efectsVolumeLinearBase = 1.0;

    void setMusicVolumeLog(int logVolume)
    {
        m_musicVolumeLog        = logVolume;
        m_musicVolumeLinearBase = QAudio::convertVolume(m_musicVolumeLog / qreal(100),
                                                        QAudio::LogarithmicVolumeScale,
                                                        QAudio::LinearVolumeScale);
        qreal approxVolume      = 100. * m_musicVolumeLinearBase;
        m_musicVolumeFinal      = static_cast<int>(approxVolume);
        if (m_musicVolumeFinal < 1 && m_musicVolumeLog > 0)
            m_musicVolumeFinal = 1;
        Logger(Logger::Notice) << "setMusicVolume=" << m_musicVolumeLog << " / " << m_musicVolumeFinal;
    }
    void setEffectsVolumeLog(int logVolume)
    {
        m_efectsVolumeLog        = logVolume;
        m_efectsVolumeLinearBase = QAudio::convertVolume(m_efectsVolumeLog / qreal(100),
                                                         QAudio::LogarithmicVolumeScale,
                                                         QAudio::LinearVolumeScale);
        Logger(Logger::Notice) << "setEffectsVolume=" << m_efectsVolumeLog << " / " << m_efectsVolumeLinearBase * 100;
    }

    QSoundEffect* getEffect(const std::string& key, int maxLimit, QObject* parent)
    {
        auto& cacheRecord = m_effectsCache[key];
        if (cacheRecord.m_effects.size() < maxLimit) {
            QSoundEffect* eff = new QSoundEffect(parent);
            cacheRecord.m_effects << eff;
            cacheRecord.m_lastIndex = cacheRecord.m_effects.size() - 1;
            return eff;
        } else {
            cacheRecord.m_lastIndex++;
            cacheRecord.m_lastIndex %= cacheRecord.m_effects.size();
            QSoundEffect* eff = cacheRecord.m_effects[cacheRecord.m_lastIndex];
            return eff;
        }
    }

    Impl()
    {
        m_player.setVolume(0);
    }
};

class MusicResource : public ISoundResource {
public:
    MusicResource(MusicBox& parent, const IMusicBox::MusicSettings& settings)
        : m_parent(parent)
        , m_settings(settings)
    {}

    void play() const override
    {
        m_parent.musicPlay(m_settings);
    }
    void playFor(int) const override
    {
        m_parent.musicPlay(m_settings);
    }

private:
    MusicBox&                m_parent;
    IMusicBox::MusicSettings m_settings;
};
class EffectResource : public ISoundResource {
public:
    EffectResource(MusicBox& parent, const IMusicBox::EffectSettings& settings)
        : m_parent(parent)
        , m_settings(settings)
    {}

    void play() const override
    {
        m_parent.effectPlay(m_settings);
    }
    void playFor(int expectedDurationMS) const override
    {
        auto settings = m_settings;
        m_parent.effectPlay(settings.setExpectedDuration(expectedDurationMS));
    }

private:
    MusicBox&                 m_parent;
    IMusicBox::EffectSettings m_settings;
};

MusicBox::MusicBox(Core::IRandomGenerator& rng, Core::IResourceLibrary& resourceLibrary)
    : m_impl(std::make_unique<Impl>())
    , m_rng(rng)
    , m_resourceLibrary(resourceLibrary)
{
    m_impl->m_player.setPlaylist(&m_impl->m_playlist);
}

MusicBox::~MusicBox()
{
}

ISoundResourcePtr MusicBox::musicPrepare(const IMusicBox::MusicSettings& music)
{
    return std::make_shared<MusicResource>(*this, music);
}

ISoundResourcePtr MusicBox::effectPrepare(const IMusicBox::EffectSettings& effect)
{
    return std::make_shared<EffectResource>(*this, effect);
}

void MusicBox::musicPlay(const IMusicBox::MusicSettings& music)
{
    std::string id = music.resourceId;
    if (id.empty()) {
        static const std::vector<std::string> varsCombat{ "01", "02", "03", "04" };
        uint8_t                               combatSize = varsCombat.size() - 1;
        switch (music.mSet) {
            case MusicSet::Battle:
            {
                uint8_t index = m_rng.genSmall(combatSize);
                id            = "combat" + varsCombat[index];
            } break;
            case MusicSet::Intro:
                id = "mainmenu";
                break;
            case MusicSet::Win:
                id = "win battle";
                break;
            case MusicSet::Lose:
                id = "losecombat";
                break;
            default:
                break;
        }
    }
    if (!m_resourceLibrary.mediaExists(Core::ResourceMedia::Type::Music, id))
        return;

    auto                 record   = m_resourceLibrary.getMedia(Core::ResourceMedia::Type::Music, id);
    const Core::std_path fullPath = record.getFullPath();
    if (fullPath.empty())
        return;

    const QString path = QString::fromStdString(Core::path2string(fullPath));
    m_impl->m_player.setVolume(0);

    m_impl->m_playlist.clear();
    m_impl->m_playlist.addMedia(QMediaContent(QUrl::fromLocalFile(path)));
    m_impl->m_playlist.setPlaybackMode(music.loopAround ? QMediaPlaylist::Loop : QMediaPlaylist::CurrentItemOnce);
    m_impl->m_player.play();
    QPropertyAnimation* volumeAnim = new QPropertyAnimation(&m_impl->m_player, "volume", this);
    volumeAnim->setEndValue(m_impl->m_musicVolumeFinal);
    volumeAnim->setDuration(std::max(music.fadeInMs, 1));

    volumeAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MusicBox::effectPlay(const EffectSettings& effect)
{
    std::string id = effect.resourceId;
    if (id.empty()) {
        if (effect.commonEffect == EffectSet::Battle) {
            std::vector<std::string> suffixes{ "00", "01", "02", "03", "04", "05", "06", "07" };
            id = "battle" + suffixes[m_rng.genSmall(suffixes.size() - 1)];
        } else if (effect.commonEffect == EffectSet::Click) {
            id = "button";
        }
    }
    if (!m_resourceLibrary.mediaExists(Core::ResourceMedia::Type::Sound, id))
        return;

    auto                 record   = m_resourceLibrary.getMedia(Core::ResourceMedia::Type::Sound, id);
    const Core::std_path fullPath = record.getFullPath();
    if (fullPath.empty())
        return;

    const QString path                   = QString::fromStdString(Core::path2string(fullPath));
    const int     maxSimultaneousEffects = 3;
    QSoundEffect* qeffect                = m_impl->getEffect(id, maxSimultaneousEffects, this);
    qeffect->disconnect();
    qeffect->setSource(QUrl::fromLocalFile(path));
    const int fadeIn       = effect.fadeIn;
    qreal     targetVolume = m_impl->m_efectsVolumeLinearBase;
    qeffect->setVolume(fadeIn > 0 ? 0 : targetVolume);
    if (effect.loopAround)
        qeffect->setLoopCount(QSoundEffect::Infinite);

    const int delay = std::max(1, effect.delay);

    QTimer::singleShot(delay, [qeffect, fadeIn, targetVolume] {
        qeffect->play();
        qeffect->setMuted(false);
        if (fadeIn > 0) {
            QPropertyAnimation* volumeAnim = new QPropertyAnimation(qeffect, "volume", qeffect);
            volumeAnim->setEndValue(targetVolume);
            volumeAnim->setDuration(fadeIn);
            volumeAnim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    });

    if (effect.expectedDuration > 0) {
        const int defaultFadeout = 100;
        const int fadeoutLength  = effect.fadeOut ? effect.fadeOut : defaultFadeout;
        const int stopAfter      = effect.expectedDuration + delay;
        const int startFadeout   = stopAfter - fadeoutLength;
        QTimer::singleShot(startFadeout, this, [qeffect, fadeoutLength] {
            QPropertyAnimation* volumeAnim = new QPropertyAnimation(qeffect, "volume", qeffect);
            volumeAnim->setEndValue(0);
            volumeAnim->setDuration(fadeoutLength);
            volumeAnim->start(QAbstractAnimation::DeleteWhenStopped);
        });
        QTimer::singleShot(stopAfter, this, [qeffect] {
            qeffect->setMuted(true);
        });
    }
}

void MusicBox::setMusicVolume(int percent)
{
    m_impl->setMusicVolumeLog(percent);
    m_impl->m_player.setVolume(m_impl->m_musicVolumeFinal);
}

void MusicBox::setEffectsVolume(int percent)
{
    m_impl->setEffectsVolumeLog(percent);
}

int MusicBox::getMusicVolume() const noexcept
{
    return m_impl->m_musicVolumeLog;
}

int MusicBox::getEffectsVolume() const noexcept
{
    return m_impl->m_efectsVolumeLog;
}

}
