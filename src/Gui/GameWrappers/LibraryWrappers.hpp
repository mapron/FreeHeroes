/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "GuiGameWrappersExport.hpp"

#include "IGuiResource.hpp"
#include "ISoundResource.hpp"

#include "LibraryFwd.hpp"
#include "Stat.hpp"

#include <QString>
#include <QPixmap>
#include <QObject>

#include <array>

namespace FreeHeroes::Sound {
class IMusicBox;
}

namespace FreeHeroes::Gui {

class IGraphicsLibrary;

template<typename WrapperTypeT, typename SrcTypeT>
class GUIGAMEWRAPPERS_EXPORT AbstractGuiWrapper {
public:
    using WrapperType = WrapperTypeT;
    using SrcType     = SrcTypeT;
    using SrcTypePtr  = const SrcType*;
    AbstractGuiWrapper(SrcTypePtr source)
        : m_source(source)
    {
    }
    SrcTypePtr getSource() const noexcept { return m_source; }

    QString getName(int n = -1) const;

private:
    SrcTypePtr const m_source;
};

class GUIGAMEWRAPPERS_EXPORT GuiArtifact : public QObject
    , public AbstractGuiWrapper<GuiArtifact, Core::LibraryArtifact> {
    using Base = AbstractGuiWrapper<GuiArtifact, Core::LibraryArtifact>;

public:
    GuiArtifact(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, Core::LibraryArtifactConstPtr source);

    QString getName(int n = -1) const;
    QString getDescr() const;

    QPixmap getIconStash() const { return m_iconStash->get(); }
    QPixmap getIconBonus() const { return m_iconBonus->get(); }

    QString localizedPartSetDescr(int setParts = -1, Core::LibraryArtifactConstPtr missingArtifact = nullptr) const;
    QString localizedDisassemblySetDescr() const;

private:
    IAsyncPixmapPtr m_iconStash;
    IAsyncPixmapPtr m_iconBonus;
};
using GuiArtifactConstPtr = const GuiArtifact*;

class GUIGAMEWRAPPERS_EXPORT GuiUnit : public QObject
    , public AbstractGuiWrapper<GuiUnit, Core::LibraryUnit> {
    Q_OBJECT
    using Base = AbstractGuiWrapper<GuiUnit, Core::LibraryUnit>;

public:
    GuiUnit(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, Core::LibraryUnitConstPtr source);

    enum class Variation
    {
        Normal,
        AsTarget
    };
    QString getNameWithCount(int n, Variation variation = Variation::Normal) const;

    QPixmap   getPortraitSmall() const { return m_portraitSmall->get(); }
    QPixmap   getPortraitLarge() const { return m_portraitLarge->get(); }
    SpritePtr getBattleSprite() const { return m_battleSprite->get(); }
    SpritePtr getProjectileSprite() const { return m_projectileSprite->get(); }
    QIcon     getSplashControl() const;

private:
    IAsyncPixmapPtr m_portraitSmall;
    IAsyncPixmapPtr m_portraitLarge;

    IAsyncSpritePtr m_battleSprite;
    IAsyncSpritePtr m_projectileSprite;

    IAsyncIconPtr m_splashControl;
};
using GuiUnitConstPtr = const GuiUnit*;
class GuiUnitProvider {
public:
    virtual ~GuiUnitProvider()                                           = default;
    virtual GuiUnitConstPtr find(Core::LibraryUnitConstPtr source) const = 0;
};

class GUIGAMEWRAPPERS_EXPORT GuiHero : public QObject
    , public AbstractGuiWrapper<GuiHero, Core::LibraryHero> {
    Q_OBJECT
    using Base = AbstractGuiWrapper<GuiHero, Core::LibraryHero>;

public:
    GuiHero(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, Core::LibraryHeroConstPtr source);

    QPixmap getPortraitSmall() const { return m_portraitSmall->get(); }
    QPixmap getPortraitLarge() const { return m_portraitLarge->get(); }
    QPixmap getSpecIcon() const { return m_specIcon->get(); }

    SpritePtr getBattleSprite() const { return m_battleSprite->get(); }
    SpritePtr getAdventureSprite() const { return m_adventureSprite->get(); }

    QString getClassName() const;
    QString getBio() const;
    QString getSpecName() const;

private:
    IAsyncPixmapPtr m_portraitSmall;
    IAsyncPixmapPtr m_portraitLarge;
    IAsyncPixmapPtr m_specIcon;
    IAsyncSpritePtr m_battleSprite;
    IAsyncSpritePtr m_adventureSprite;
};
using GuiHeroConstPtr = const GuiHero*;
class GuiHeroProvider {
public:
    virtual ~GuiHeroProvider()                                           = default;
    virtual GuiHeroConstPtr find(Core::LibraryHeroConstPtr source) const = 0;
};

class GUIGAMEWRAPPERS_EXPORT GuiSkill : public QObject
    , public AbstractGuiWrapper<GuiSkill, Core::LibrarySecondarySkill> {
    Q_OBJECT
    using Base = AbstractGuiWrapper<GuiSkill, Core::LibrarySecondarySkill>;

public:
    GuiSkill(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, Core::LibrarySecondarySkillConstPtr source);

    QPixmap getIconSmall(int level) const { return m_iconSmall[level]->get(); }
    QPixmap getIconMedium(int level) const { return m_iconMedium[level]->get(); }
    QPixmap getIconLarge(int level) const { return m_iconLarge[level]->get(); }

    QString getDescription(int level) const;

    static QString getSkillLevelName(int level);

private:
    std::array<IAsyncPixmapPtr, 3> m_iconSmall;
    std::array<IAsyncPixmapPtr, 3> m_iconMedium;
    std::array<IAsyncPixmapPtr, 3> m_iconLarge;
};
using GuiSkillConstPtr = const GuiSkill*;
class GuiSkillProvider {
public:
    virtual ~GuiSkillProvider()                                                     = default;
    virtual GuiSkillConstPtr find(Core::LibrarySecondarySkillConstPtr source) const = 0;
};

class GUIGAMEWRAPPERS_EXPORT GuiSpell : public QObject
    , public AbstractGuiWrapper<GuiSpell, Core::LibrarySpell> {
    Q_OBJECT
    using Base = AbstractGuiWrapper<GuiSpell, Core::LibrarySpell>;

public:
    GuiSpell(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, Core::LibrarySpellConstPtr source);

    QPixmap getIconBonus() const { return m_iconBonus->get(); }
    QPixmap getIconInt() const { return m_iconInt->get(); }
    QPixmap getIconTrans() const { return m_iconTrans->get(); }
    QPixmap getIconScroll() const { return m_iconScroll->get(); }

    SpritePtr getAnimation() const { return m_animation->get(); }
    SpritePtr getBottomAnimation() const { return m_bottomAnimation->get(); }
    SpritePtr getProjectile() const { return m_projectile->get(); }

    Sound::ISoundResourcePtr getSound() const { return m_sound; }

    bool hasBottomAnimation() const { return m_bottomAnimation->exists(); }
    bool hasProjectile() const { return m_projectile->exists(); }

    QString getDescription(int level, int hintDamage) const;

    QString spellBookInfo(int manaCost, bool shortFormat) const;

private:
    IAsyncPixmapPtr m_iconBonus;
    IAsyncPixmapPtr m_iconInt;
    IAsyncPixmapPtr m_iconTrans;
    IAsyncPixmapPtr m_iconScroll;

    IAsyncSpritePtr m_animation;
    IAsyncSpritePtr m_bottomAnimation;
    IAsyncSpritePtr m_projectile;

    Sound::ISoundResourcePtr m_sound;
    //std::string sound;
};
using GuiSpellConstPtr = const GuiSpell*;
class GuiSpellProvider {
public:
    virtual ~GuiSpellProvider()                                            = default;
    virtual GuiSpellConstPtr find(Core::LibrarySpellConstPtr source) const = 0;
};

class GUIGAMEWRAPPERS_EXPORT GuiFaction : public QObject
    , public AbstractGuiWrapper<GuiFaction, Core::LibraryFaction> {
    Q_OBJECT
    using Base = AbstractGuiWrapper<GuiFaction, Core::LibraryFaction>;

public:
    GuiFaction(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, Core::LibraryFactionConstPtr source);

    SpritePtr getUnitBackground() const { return m_unitBackground->get(); }

private:
    IAsyncSpritePtr m_unitBackground;
};
using GuiFactionConstPtr = const GuiFaction*;
class GuiFactionProvider {
public:
    virtual ~GuiFactionProvider()                                              = default;
    virtual GuiFactionConstPtr find(Core::LibraryFactionConstPtr source) const = 0;
};

class GUIGAMEWRAPPERS_EXPORT GuiTerrain : public QObject
    , public AbstractGuiWrapper<GuiTerrain, Core::LibraryTerrain> {
    Q_OBJECT
    using Base = AbstractGuiWrapper<GuiTerrain, Core::LibraryTerrain>;

public:
    GuiTerrain(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, Core::LibraryTerrainConstPtr source);

    QPixmap getIcon() const { return m_icon->get(); }
    QPixmap getTile(int variant) const;

private:
    IAsyncPixmapPtr         m_icon;
    const IGraphicsLibrary* m_graphicsLibrary;
};
using GuiTerrainConstPtr = const GuiTerrain*;
class GuiTerrainProvider {
public:
    virtual ~GuiTerrainProvider()                                              = default;
    virtual GuiTerrainConstPtr find(Core::LibraryTerrainConstPtr source) const = 0;
};

class GUIGAMEWRAPPERS_EXPORT GuiMapBank : public QObject
    , public AbstractGuiWrapper<GuiMapBank, Core::LibraryMapBank> {
    Q_OBJECT
    using Base = AbstractGuiWrapper<GuiMapBank, Core::LibraryMapBank>;

public:
    GuiMapBank(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, Core::LibraryMapBankConstPtr source);

    QPixmap            getIcon() const { return m_icon->get(); }
    const QStringList& getVariantNames() const { return m_variantNames; }

private:
    IAsyncPixmapPtr m_icon;
    QStringList     m_variantNames;
};
using GuiMapBankConstPtr = const GuiMapBank*;
class GuiMapBankProvider {
public:
    virtual ~GuiMapBankProvider()                                              = default;
    virtual GuiMapBankConstPtr find(Core::LibraryMapBankConstPtr source) const = 0;
};

class GUIGAMEWRAPPERS_EXPORT ResourceAmountHelper : public QObject {
    Q_OBJECT
public:
    struct ResourceInfo {
        QString id;
        QString name;
        int     amount = 0;
    };
    QList<ResourceInfo> trasformResourceAmount(const Core::ResourceAmount& resourceAmount) const;

    QString resourceName(const std::string& resourceId) const;
};

}
