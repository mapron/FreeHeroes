/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "ISoundResource.hpp"

#include <string>

namespace FreeHeroes::Sound {

class IMusicBox {
public:
    // clang-format off
    enum class MusicSet { None, Intro, Battle, Win, Lose };
    enum class EffectSet { None, Click , Battle};
    // clang-format on

    struct EffectSettings {
        std::string resourceId;
        EffectSet   commonEffect = EffectSet::None;

        bool loopAround       = false;
        int  fadeIn           = 0;
        int  fadeOut          = 0;
        int  expectedDuration = 0;
        int  delay            = 0;
        EffectSettings(const std::string& id)
            : resourceId(id)
        {}
        EffectSettings(EffectSet e)
            : commonEffect(e)
        {}

        EffectSettings& setLoopAround(bool ploopAround = true)
        {
            this->loopAround = ploopAround;
            return *this;
        }
        EffectSettings& setFadeIn(int pfadeIn)
        {
            this->fadeIn = pfadeIn;
            return *this;
        }
        EffectSettings& setFadeOut(int pfadeOut)
        {
            this->fadeOut = pfadeOut;
            return *this;
        }
        EffectSettings& setExpectedDuration(int pexpectedDuration)
        {
            this->expectedDuration = pexpectedDuration;
            return *this;
        }
        EffectSettings& setDelay(int pdelay)
        {
            this->delay = pdelay;
            return *this;
        }
    };

    struct MusicSettings {
        int         delayMs    = 0;
        int         fadeInMs   = 2000;
        int         fadeOutMs  = 2000;
        bool        loopAround = true;
        MusicSet    mSet       = MusicSet::None;
        std::string resourceId;
        MusicSettings(const std::string& id)
            : resourceId(id)
        {}
        MusicSettings(MusicSet m)
            : mSet(m)
        {}

        MusicSettings& setLoopAround(bool ploopAround = true)
        {
            this->loopAround = ploopAround;
            return *this;
        }
        MusicSettings& setFadeIn(int fadeIn)
        {
            this->fadeInMs = fadeIn;
            return *this;
        }
        MusicSettings& setFadeOut(int fadeOut)
        {
            this->fadeOutMs = fadeOut;
            return *this;
        }
        MusicSettings& setDelay(int delay)
        {
            this->delayMs = delay;
            return *this;
        }
    };

public:
    virtual ~IMusicBox() = default;

    virtual ISoundResourcePtr musicPrepare(const MusicSettings& music) = 0;

    virtual ISoundResourcePtr effectPrepare(const EffectSettings& effect) = 0;

    virtual void setMusicVolume(int percent)   = 0;
    virtual void setEffectsVolume(int percent) = 0;

    virtual int getMusicVolume() const noexcept   = 0;
    virtual int getEffectsVolume() const noexcept = 0;
};

}
