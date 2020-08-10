/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QString>
#include <QSettings>

class QWidget;

namespace FreeHeroes::Gui {

class IAppSettings {
public:
    virtual ~IAppSettings() = default;

    struct Sound {
        int musicVolumePercent = 20;
        int effectsVolumePercent = 20;
    };

    struct Battle {
        int walkTimePercent = 100;
        int otherTimePercent = 100;
        int shiftSpeedPercent = 50;
        bool displayGrid = true;
        bool displayPath = false;
        bool logMoves = false;
        bool retaliationHint = true;
    };

    struct AppGlobal {
        int logLevel = 6;
        QString localeId = "";
        QStringList localeItems;
        QString databaseId = "FH_HotA";
        QStringList databaseItems;
    };

    struct UI {
        bool displayAbsMoraleLuck = true;
        bool clampAbsMoraleLuck = true;
    };

    struct AllSettings {
        Sound sound;
        Battle battle;
        AppGlobal global;
        UI ui;
    };

    virtual const Sound & sound() const noexcept = 0;

    virtual const Battle & battle() const noexcept = 0;

    virtual const AppGlobal & global() const noexcept = 0;

    virtual const UI & ui() const noexcept = 0;

    virtual QSettings & uiSettings() = 0;

    virtual void showSettingsEditor(QWidget * parent) = 0;
};

}
