/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QString>
#include <QSettings>

#include "GameConstants.hpp"

class QWidget;

namespace FreeHeroes::Gui {

class IAppSettings {
public:
    virtual ~IAppSettings() = default;

    struct Sound {
        int musicVolumePercent   = 20;
        int effectsVolumePercent = 20;
    };

    struct Battle {
        int  walkTimePercent   = 100;
        int  otherTimePercent  = 100;
        int  shiftSpeedPercent = 50;
        bool displayGrid       = true;
        bool displayPath       = false;
        bool logMoves          = false;
        bool retaliationHint   = true;
        bool massDamageHint    = true;
        bool counterDamageHint = true;
    };

    struct AppGlobal {
        int         logLevel = 6;
        QString     localeId = "";
        QStringList localeItems;
        QString     databaseIdList = QString{ Core::g_database_HOTA };
        QString     resourcesList;

        void reset()
        {
            databaseIdList = QString{ Core::g_database_HOTA };
        }
        std::vector<std::string> getDbIds() const
        {
            std::vector<std::string> res;
            auto                     parts = databaseIdList.split(',');
            for (const auto& part : parts)
                res.push_back(part.toStdString());
            return res;
        }
        std::vector<std::string> getResourceIds() const
        {
            std::vector<std::string> res;
            auto                     parts = resourcesList.split(',');
            for (const auto& part : parts)
                res.push_back(part.toStdString());
            return res;
        }
    };

    struct UI {
        bool displayAbsMoraleLuck = true;
        bool clampAbsMoraleLuck   = true;
    };

    struct AllSettings {
        Sound     sound;
        Battle    battle;
        AppGlobal global;
        UI        ui;
    };

    virtual const Sound& sound() const noexcept = 0;

    virtual const Battle& battle() const noexcept = 0;

    virtual const AppGlobal& global() const noexcept = 0;

    virtual const UI& ui() const noexcept = 0;

    virtual QSettings& uiSettings() = 0;

    virtual void showSettingsEditor(QWidget* parent) = 0;
};

}
