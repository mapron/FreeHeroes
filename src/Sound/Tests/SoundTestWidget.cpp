/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SoundTestWidget.hpp"

#include "IMusicBox.hpp"

#include <QBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

namespace FreeHeroes {

SoundTestWidget::SoundTestWidget(Sound::IMusicBox& musicBox)
    : QDialog(nullptr)
{
    QVBoxLayout * layout = new QVBoxLayout(this);
    {
        QHBoxLayout * layoutMusic = new QHBoxLayout();
        layout->addLayout(layoutMusic);
        QPushButton * pbPlayMusic = new QPushButton("Play music", this);
        layoutMusic->addWidget(pbPlayMusic);
        layoutMusic->addWidget(new QLabel("Music volume:", this));
        auto musicVolumeSlider = new QSlider(this);
        musicVolumeSlider->setOrientation(Qt::Horizontal);
        musicVolumeSlider->setMaximum(100);
        musicVolumeSlider->setValue(musicBox.getMusicVolume());
        layoutMusic->addWidget(musicVolumeSlider);
        layoutMusic->addStretch();

        QList<Sound::IMusicBox::MusicSettings> musicList;
        musicList << Sound::IMusicBox::MusicSettings{Sound::IMusicBox::MusicSet::Intro};
        musicList << Sound::IMusicBox::MusicSettings{Sound::IMusicBox::MusicSet::Battle};
        musicList << Sound::IMusicBox::MusicSettings{Sound::IMusicBox::MusicSet::Win}.setLoopAround(false);
        musicList << Sound::IMusicBox::MusicSettings{"rough"};
        int currentMusicIndex = -1;

        connect(pbPlayMusic, &QPushButton::clicked, this, [&musicBox, currentMusicIndex, musicList]() mutable {
            currentMusicIndex++;
            currentMusicIndex = currentMusicIndex % musicList.size();
            musicBox.musicPrepare(musicList[currentMusicIndex])->play();
        });
        connect(musicVolumeSlider, &QSlider::sliderReleased, this, [musicVolumeSlider, &musicBox]{
            musicBox.setMusicVolume(musicVolumeSlider->value());
        });
    }

    {
        QHBoxLayout * layoutEffects = new QHBoxLayout();
        layout->addLayout(layoutEffects);
        QPushButton * pbPlayEffect = new QPushButton("Play next effect group", this);
        layoutEffects->addWidget(pbPlayEffect);
        layoutEffects->addWidget(new QLabel("Effects volume:", this));
        auto effectsVolumeSlider = new QSlider(this);
        effectsVolumeSlider->setOrientation(Qt::Horizontal);
        effectsVolumeSlider->setMaximum(100);
        effectsVolumeSlider->setValue(musicBox.getEffectsVolume());
        layoutEffects->addWidget(effectsVolumeSlider);
        layoutEffects->addStretch();

        using EffectsSettingsList = QList<Sound::IMusicBox::EffectSettings> ;
        QList<EffectsSettingsList> effectGroupList;
        {
            EffectsSettingsList group;
            group << Sound::IMusicBox::EffectSettings{Sound::IMusicBox::EffectSet::Click};
            effectGroupList << group;
        }
        {
            EffectsSettingsList group;
            group << Sound::IMusicBox::EffectSettings{Sound::IMusicBox::EffectSet::Click};
            effectGroupList << group;
        }
        {
            EffectsSettingsList group;
            group << Sound::IMusicBox::EffectSettings{Sound::IMusicBox::EffectSet::Battle};
            effectGroupList << group;
        }
        {
            EffectsSettingsList group;
            group << Sound::IMusicBox::EffectSettings{"cavamove"}
                     .setLoopAround(true)
                     .setDelay(300)
                     .setFadeIn(1000)
                     .setFadeOut(1000)
                     .setExpectedDuration(3000);
            effectGroupList << group;
        }
        {
            EffectsSettingsList group;
            group << Sound::IMusicBox::EffectSettings{"pfndmove"}.setLoopAround(true).setExpectedDuration(1000);
            effectGroupList << group;
        }
        {
            EffectsSettingsList group;
            group << Sound::IMusicBox::EffectSettings{"aelmattk"}.setLoopAround(false);
            group << Sound::IMusicBox::EffectSettings{"plizkill"}.setLoopAround(false);
            group << Sound::IMusicBox::EffectSettings{"cavakill"}.setLoopAround(false);
            group << Sound::IMusicBox::EffectSettings{"sod.spell.meteor"}.setLoopAround(false);
            effectGroupList << group;
        }


        int currentEffectIndex = -1;

        connect(pbPlayEffect, &QPushButton::clicked, this, [&musicBox, currentEffectIndex, effectGroupList]() mutable {
            currentEffectIndex++;
            currentEffectIndex = currentEffectIndex % effectGroupList.size();
            for (const auto & eff : effectGroupList[currentEffectIndex])
                musicBox.effectPrepare(eff)->play();
        });
        connect(effectsVolumeSlider, &QSlider::sliderReleased, this, [effectsVolumeSlider, &musicBox]{
            musicBox.setEffectsVolume(effectsVolumeSlider->value());
        });
    }

    layout->addStretch();

}



}
