/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "SoundPlayerExport.hpp"

#include "IMusicBox.hpp"

#include "IResourceLibrary.hpp"
#include "IRandomGenerator.hpp"

#include <QObject>

namespace FreeHeroes::Sound {

class SOUNDPLAYER_EXPORT MusicBox : public IMusicBox
    , public QObject {
public:
    MusicBox(Core::IRandomGenerator& rng, Core::IResourceLibrary& resourceLibrary);
    ~MusicBox();

public:
    ISoundResourcePtr musicPrepare(const MusicSettings& music) override;

    ISoundResourcePtr effectPrepare(const EffectSettings& effect) override;

    void musicPlay(const MusicSettings& music);
    void effectPlay(const EffectSettings& effect);

    void setMusicVolume(int percent) override;
    void setEffectsVolume(int percent) override;

    int getMusicVolume() const noexcept override;
    int getEffectsVolume() const noexcept override;

private:
private:
    struct Impl;
    std::unique_ptr<Impl>   m_impl;
    Core::IRandomGenerator& m_rng;
    Core::IResourceLibrary& m_resourceLibrary;
};

}
