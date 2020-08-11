/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiGameWrappersExport.hpp"

#include "IGuiResource.hpp"
#include "ISoundResource.hpp"

#include "Stat.hpp"

#include <QObject>
#include <QMap>

namespace FreeHeroes::Sound
{
class IMusicBox;
}

namespace FreeHeroes::Gui {
class IGraphicsLibrary;

/// @todo: not really a model! Need to prettify this.
class GUIGAMEWRAPPERS_EXPORT UiCommonModel : public QObject
{
    Q_OBJECT
public:
    UiCommonModel(Sound::IMusicBox & musicBox,
                  IGraphicsLibrary & graphicsLibrary,
                  QObject * parent);
    ~UiCommonModel();

    enum class UIString { Close, Cancel };
    QString getCommonString(UIString common) const;

    QStringList getLuckDescription(const Core::LuckDetails & details) const;
    QStringList getMoraleDescription(const Core::MoraleDetails & details) const;

    struct PrimarySkillInfo {
        QString name;
        QString descr;
        IAsyncPixmapPtr iconSmall;
        IAsyncPixmapPtr iconMedium;
        IAsyncPixmapPtr iconLarge;
    };
    QMap<Core::HeroPrimaryParamType, PrimarySkillInfo> skillInfo;

    struct Spellbook {
        IAsyncPixmapPtr background;
        IAsyncPixmapPtr prevPage;
        IAsyncPixmapPtr nextPage;
        QList<IAsyncPixmapPtr> spelltabs;
        QList<IAsyncPixmapPtr> spellTitles;
    };
    Spellbook spellbook;

    IAsyncPixmapPtr disbandStack;

    struct Buttons {
        IAsyncIconPtr close;
        IAsyncIconPtr okWide; // like close but wider
        IAsyncIconPtr cancel;

        IAsyncIconPtr scrollLeft;
        IAsyncIconPtr scrollRight;
    };
    Buttons buttons;

    struct HeroDialog {
        IAsyncIconPtr distantIcons;
        IAsyncIconPtr compactIcons;
        IAsyncIconPtr tacticsIcons;
        IAsyncIconPtr stackSplit;

        IAsyncIconPtr deleteHero;
        IAsyncIconPtr bag;
        IAsyncIconPtr listInfo;

        IAsyncPixmapPtr labelFlag;
    };
    HeroDialog heroDialog;

    struct RngIcons {
        QMap<int, IAsyncPixmapPtr> large;
        QMap<int, IAsyncPixmapPtr> medium;
        QMap<int, IAsyncPixmapPtr> small;
    };
    RngIcons luck;
    RngIcons morale;

    struct BattleResult {
        IAsyncMoviePtr anim;
        Sound::ISoundResourcePtr music;
    };
    BattleResult win;
    BattleResult lose;
    struct BattleControl {
        IAsyncIconPtr wait;
        IAsyncIconPtr guard;
        IAsyncIconPtr spellBook;
        IAsyncIconPtr surrender;
        IAsyncIconPtr autoCombat;
        IAsyncIconPtr settings;

        IAsyncIconPtr unitCast;
        IAsyncIconPtr rangeAttack;
        IAsyncIconPtr meleeAttack;
    };
    BattleControl battleControl;

    QMap<QString, QPixmap> resourceIcons;
    QMap<QString, QPixmap> resourceIconsSmall;

    Sound::ISoundResourcePtr clickEffect;
};

}
