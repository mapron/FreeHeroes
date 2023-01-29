/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LibraryWrappers.hpp"

#include "IGraphicsLibrary.hpp"

#include "LibraryArtifact.hpp"
#include "LibraryHero.hpp"
#include "LibraryHeroSpec.hpp"
#include "LibraryMapBank.hpp"
#include "LibraryObjectDef.hpp"
#include "LibraryPlayer.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryUnit.hpp"

#include "IMusicBox.hpp"

#include <QCoreApplication>
#include <QIcon>

namespace FreeHeroes::Gui {

namespace {

QString translateHelper(const Core::TranslationMap& tsMap,
                        const std::string&          untranslatedName,
                        const std::string&          key,
                        int                         n = -1)
{
    if (tsMap.ts.contains("ru_RU")) {
        return QString::fromStdString(tsMap.ts.at("ru_RU"));
    }
    if (tsMap.ts.contains("de_DE")) {
        return QString::fromStdString(tsMap.ts.at("de_DE"));
    }
    if (tsMap.ts.contains("en_US")) {
        return QString::fromStdString(tsMap.ts.at("en_US"));
    }

    return untranslatedName.empty() ? QString("$(%1)").arg(key.c_str()) : QString::fromStdString(untranslatedName);
}

template<typename T>
QString translateHelper(const T* obj, int n = -1)
{
    return translateHelper(obj->presentationParams.name, obj->untranslatedName, obj->id, n);
}

QString prepareDescription(QString description)
{
    const QString formatTitleBegin = QString("<font color=\"#EDD57A\">");
    const QString formatTitleEnd   = QString("</font>");
    if (description.startsWith("\""))
        description = description.mid(1);

    if (description.endsWith("\""))
        description = description.mid(0, description.size() - 1);

    description.replace("\\n", "<br>");
    description.replace("{", formatTitleBegin);
    description.replace("}", formatTitleEnd);
    return description;
}

IGraphicsLibrary::PixmapKey getTerrainKey(Core::LibraryTerrainConstPtr terrain, int variant)
{
    return IGraphicsLibrary::PixmapKey(terrain->presentationParams.defFile, variant, 0);
}

}

template<typename WrapperType, typename SrcType>
QString AbstractGuiWrapper<WrapperType, SrcType>::getName(int n) const
{
    QString name = translateHelper(getSource(), n);
    return name;
}

GuiArtifact::GuiArtifact(Sound::IMusicBox*, const IGraphicsLibrary* graphicsLibrary, Core::LibraryArtifactConstPtr source)
    : QObject(nullptr)
    , Base(source)
    , m_iconStash(graphicsLibrary->getPixmap(source->presentationParams.iconStash))
    , m_iconBonus(graphicsLibrary->getPixmap(source->presentationParams.iconBonus))
{
}

QString GuiArtifact::getName(int n) const
{
    if (getSource()->scrollSpell) {
        QString spellName  = translateHelper(getSource()->scrollSpell);
        QString scrollName = "SCROLL";
        //QString scrollName = translateHelper(TranslationContextName<Core::LibraryArtifact>::context, "", "sod.artifact.special.scroll", n);
        return scrollName + " \"" + spellName + "\"";
    }

    QString localizedName = translateHelper(getSource(), n);
    return localizedName;
}

QString GuiArtifact::getDescr() const
{
    QString localizedDesc = ""; /*= translateHelper(TranslationContextName<Core::LibraryArtifact>::context,
                                            getSource()->untranslatedName,
                                            getSource()->id + ".descr");*/
    return prepareDescription(localizedDesc);
}

QString GuiArtifact::localizedPartSetDescr(int setParts, Core::LibraryArtifactConstPtr missingArtifact) const
{
    QString localizedDescr;
    localizedDescr += tr("This artifacts is a part of the set: ") + "<br>" + "{" + getName() + "}";
    if (setParts >= 0) {
        localizedDescr += "<br>";
        localizedDescr += tr("Parts of set assemblied: {%1} / {%2}").arg(setParts).arg(getSource()->parts.size());
        if (missingArtifact) {
            localizedDescr += "<br>";
            localizedDescr += tr("Last remaining artifact: ") + "<br>{" + translateHelper(missingArtifact) + "}";
        }
        if (setParts == (int) getSource()->parts.size()) {
            localizedDescr += "<br><br>" + tr("Congratulations! You found all parts of the set!<br>For assembly, press {Control+Left Click}");
        }
    }
    return prepareDescription(localizedDescr);
}

QString GuiArtifact::localizedDisassemblySetDescr() const
{
    QString localizedDescr;
    localizedDescr += tr("This artifact can be disassembled into: ") + "<br>";
    for (auto art : getSource()->parts)
        localizedDescr += "{" + translateHelper(art) + "}<br>";
    localizedDescr += "<br>" + tr("Press {Control+Left Click} to disassemble");
    return prepareDescription(localizedDescr);
}

GuiUnit::GuiUnit(Sound::IMusicBox*, const IGraphicsLibrary* graphicsLibrary, Core::LibraryUnitConstPtr source)
    : QObject(nullptr)
    , Base(source)
    , m_portraitSmall(graphicsLibrary->getPixmap(source->presentationParams.portraitSmall))
    , m_portraitLarge(graphicsLibrary->getPixmap(source->presentationParams.portrait))
    , m_battleSprite(graphicsLibrary->getObjectAnimation(source->presentationParams.spriteBattle))
    , m_projectileSprite(graphicsLibrary->getObjectAnimation(source->presentationParams.spriteProjectile))
{
    std::string splashButtons = source->abilities.splashButtons;
    QStringList icns          = QString::fromStdString(splashButtons).split(",");
    m_splashControl           = graphicsLibrary->getIcon(icns);
    if (!m_battleSprite->exists()) {
        const bool isWide = source->traits.large;
        m_battleSprite    = graphicsLibrary->getObjectAnimation(isWide ? "stub_unit_wide" : "stub_unit_normal");
        Q_ASSERT(m_battleSprite->exists());
    }
    if (!source->presentationParams.spriteProjectile.empty() && !m_projectileSprite->exists()) {
        m_projectileSprite = graphicsLibrary->getObjectAnimation("stub_projectile");
        Q_ASSERT(m_projectileSprite->exists());
    }
}

QString GuiUnit::getNameWithCount(int n, GuiUnit::Variation variation) const
{
    auto id = getSource()->id;
    if (variation == Variation::AsTarget)
        id = id + ".accusative";
    QString localizedName = ""; //translateHelper(TranslationContextName<Core::LibraryUnit>::context, getSource()->untranslatedName, id, n);
    if (n > 0)
        return tr("%1 %2").arg(n).arg(localizedName);

    return localizedName;
}

QIcon GuiUnit::getSplashControl() const
{
    return m_splashControl->get();
}

GuiHero::GuiHero(Sound::IMusicBox*, const IGraphicsLibrary* graphicsLibrary, Core::LibraryHeroConstPtr source)
    : QObject(nullptr)
    , Base(source)
    , m_portraitSmall(graphicsLibrary->getPixmap(source->presentationParams.portraitSmall))
    , m_portraitLarge(graphicsLibrary->getPixmap(source->presentationParams.portrait))
    , m_specIcon(graphicsLibrary->getPixmap(source->spec->presentationParams.icon))
{
    auto hClass        = source->heroClass();
    auto resNameBattle = source->presentationParams.gender == Core::LibraryHero::Presentation::Gender::Male ? hClass->presentationParams.battleSpriteMale : hClass->presentationParams.battleSpriteFemale;
    auto resNameAdv    = source->presentationParams.gender == Core::LibraryHero::Presentation::Gender::Male ? hClass->presentationParams.adventureSpriteMale : hClass->presentationParams.adventureSpriteFemale;
    m_battleSprite     = graphicsLibrary->getObjectAnimation(resNameBattle);
    if (!m_battleSprite->exists()) {
        m_battleSprite = graphicsLibrary->getObjectAnimation("stub_hero");
        Q_ASSERT(m_battleSprite->exists());
    }
    m_adventureSprite = graphicsLibrary->getObjectAnimation(resNameAdv);
    //Q_ASSERT(m_adventureSprite->exists());
}

QString GuiHero::getClassName() const
{
    return translateHelper(getSource()->heroClass());
}

QString GuiHero::getBio() const
{
    QString localizedDesc = translateHelper(getSource()->presentationParams.bio,
                                            getSource()->untranslatedName,
                                            getSource()->id + ".bio");
    return prepareDescription(localizedDesc);
}

QString GuiHero::getSpecName() const
{
    auto spec = getSource()->spec;
    if (spec->skill) {
        return translateHelper(spec->skill);
    }
    if (spec->spell) {
        return translateHelper(spec->spell);
    }
    if (spec->type == Core::LibraryHeroSpec::Type::SpecialBallista
        || spec->type == Core::LibraryHeroSpec::Type::SpecialCannon) {
        return translateHelper(spec->unit, 1);
    }
    if (spec->unit) {
        return translateHelper(spec->unit);
    }
    if (spec->type == Core::LibraryHeroSpec::Type::Income) {
        auto nameAndCount = ResourceAmountHelper().trasformResourceAmount(spec->dayIncome).value(0);
        return tr("%1 (+%2/day)").arg(nameAndCount.name).arg(nameAndCount.amount);
    }
    if (spec->type == Core::LibraryHeroSpec::Type::SpecialDragons) {
        return tr("Dragons", "speciality");
    }
    if (spec->type == Core::LibraryHeroSpec::Type::SpecialSpeed) {
        return tr("Speed", "speciality");
    }
    return QString::fromStdString(spec->id);
}

GuiSkill::GuiSkill(Sound::IMusicBox*, const IGraphicsLibrary* graphicsLibrary, Core::LibrarySecondarySkillConstPtr source)
    : QObject(nullptr)
    , Base(source)
    , m_iconSmall{ {
          graphicsLibrary->getPixmap(source->presentationParams.levels[0].iconSmall),
          graphicsLibrary->getPixmap(source->presentationParams.levels[1].iconSmall),
          graphicsLibrary->getPixmap(source->presentationParams.levels[2].iconSmall),
      } }
    , m_iconMedium{ {
          graphicsLibrary->getPixmap(source->presentationParams.levels[0].iconMedium),
          graphicsLibrary->getPixmap(source->presentationParams.levels[1].iconMedium),
          graphicsLibrary->getPixmap(source->presentationParams.levels[2].iconMedium),
      } }
    , m_iconLarge{ {
          graphicsLibrary->getPixmap(source->presentationParams.levels[0].iconLarge),
          graphicsLibrary->getPixmap(source->presentationParams.levels[1].iconLarge),
          graphicsLibrary->getPixmap(source->presentationParams.levels[2].iconLarge),
      } }
{
}

QString GuiSkill::getDescription(int level) const
{
    static const QList<std::string> suffixes{ ".basic", ".advanced", ".expert" };
    QString                         localizedDesc = ""; /*translateHelper(TranslationContextName<Core::LibrarySecondarySkill>::context,
                                            getSource()->untranslatedName,
                                            getSource()->id + suffixes.value(level));*/
    return prepareDescription(localizedDesc);
}

QString GuiSkill::getSkillLevelName(int level)
{
    static const QStringList levels{ tr("None"), tr("Basic"), tr("Advanced"), tr("Expert") };
    return levels.value(level + 1);
}

GuiSpell::GuiSpell(Sound::IMusicBox* musicBox, const IGraphicsLibrary* graphicsLibrary, Core::LibrarySpellConstPtr source)
    : QObject(nullptr)
    , Base(source)
    , m_iconBonus{ graphicsLibrary->getPixmap(source->presentationParams.iconBonus) }
    , m_iconInt{ graphicsLibrary->getPixmap(source->presentationParams.iconInt) }
    , m_iconTrans{ graphicsLibrary->getPixmap(source->presentationParams.iconTrans) }
    , m_iconScroll{ graphicsLibrary->getPixmap(source->presentationParams.iconScroll) }
    , m_animation{ graphicsLibrary->getObjectAnimation(source->presentationParams.animation) }
    , m_bottomAnimation{ graphicsLibrary->getObjectAnimation(source->presentationParams.bottomAnimation) }
    , m_projectile{ graphicsLibrary->getObjectAnimation(source->presentationParams.projectile) }
    , m_sound{ musicBox->effectPrepare(Sound::IMusicBox::EffectSettings{ source->presentationParams.sound }.setFadeIn(100).setFadeOut(100)) }
{
}

QString GuiSpell::getDescription(int level, int hintDamage) const
{
    static const QList<std::string> suffixes{ ".normal", ".basic", ".advanced", ".expert" };
    QString                         localizedDesc = ""; /* translateHelper(TranslationContextName<Core::LibrarySpell>::context,
                                            getSource()->untranslatedName,
                                            getSource()->id + suffixes.value(level));*/
    localizedDesc                                 = prepareDescription(localizedDesc);
    if (hintDamage > 0) {
        localizedDesc += "<br><br>" + tr("Inflicts damage:") + " " + QString::number(hintDamage);
    }
    return localizedDesc;
}

QString GuiSpell::spellBookInfo(int manaCost, bool shortFormat) const
{
    QString title = this->getName();
    QString descr;
    if (shortFormat)
        descr = QString("{%1} (%2 %3)").arg(title).arg(getSource()->level).arg(tr("lvl."));
    else
        descr = QString("{%1}<br>%2 %3<br>%4 %5").arg(title).arg(getSource()->level).arg(tr("lvl.")).arg(tr("Mana:")).arg(manaCost);

    return prepareDescription(descr);
}

GuiFaction::GuiFaction(Sound::IMusicBox*, const IGraphicsLibrary* graphicsLibrary, Core::LibraryFactionConstPtr source)
    : QObject(nullptr)
    , Base(source)
    , m_unitBackground(graphicsLibrary->getObjectAnimation(source->presentationParams.unitBackground))
{
}

GuiPlayer::GuiPlayer(Sound::IMusicBox*, const IGraphicsLibrary* graphicsLibrary, Core::LibraryPlayerConstPtr source)
    : QObject(nullptr)
    , Base(source)
    , m_icon(graphicsLibrary->getPixmap(source->presentationParams.icon))
    , m_graphicsLibrary(graphicsLibrary)
{
}

QColor GuiPlayer::getColor() const
{
    return QColor("#" + QString::fromStdString(getSource()->presentationParams.colorRGB));
}

GuiTerrain::GuiTerrain(Sound::IMusicBox*, const IGraphicsLibrary* graphicsLibrary, Core::LibraryTerrainConstPtr source)
    : QObject(nullptr)
    , Base(source)
    , m_icon(graphicsLibrary->getPixmapByKey(getTerrainKey(source, source->presentationParams.centerTilesOffset)))
    , m_graphicsLibrary(graphicsLibrary)
{
}

QPixmap GuiTerrain::getTile(int variant) const
{
    const auto centerTilesCount = getSource()->presentationParams.centerTilesCount;
    variant                     = variant % centerTilesCount;
    const auto key              = getTerrainKey(getSource(), getSource()->presentationParams.centerTilesOffset + variant);
    auto       terrainPixAsync  = m_graphicsLibrary->getPixmapByKey(key);
    assert(terrainPixAsync && terrainPixAsync->exists());
    auto pix = terrainPixAsync->get();
    return pix;
}

GuiMapBank::GuiMapBank(Sound::IMusicBox*, const IGraphicsLibrary* graphicsLibrary, Core::LibraryMapBankConstPtr source)
    : QObject(nullptr)
    , Base(source)
    , m_icon(graphicsLibrary->getPixmapByKey(IGraphicsLibrary::PixmapKey(source->objectDefs.get({})->id, 0, 0)))
{
    for (auto& variant : source->variants)
        m_variantNames << QString::fromStdString(variant.name);
}

QList<ResourceAmountHelper::ResourceInfo> ResourceAmountHelper::trasformResourceAmount(const Core::ResourceAmount& resourceAmount) const
{
    QList<ResourceInfo> result;
    for (const auto& [resId, count] : resourceAmount.data) {
        QString name = translateHelper(resId);
        result << ResourceInfo{ resId, name, count };
    }
    return result;
}

template class AbstractGuiWrapper<GuiArtifact, Core::LibraryArtifact>;
template class AbstractGuiWrapper<GuiFaction, Core::LibraryFaction>;
template class AbstractGuiWrapper<GuiHero, Core::LibraryHero>;
template class AbstractGuiWrapper<GuiMapBank, Core::LibraryMapBank>;
template class AbstractGuiWrapper<GuiPlayer, Core::LibraryPlayer>;
template class AbstractGuiWrapper<GuiSkill, Core::LibrarySecondarySkill>;
template class AbstractGuiWrapper<GuiSpell, Core::LibrarySpell>;
template class AbstractGuiWrapper<GuiTerrain, Core::LibraryTerrain>;
template class AbstractGuiWrapper<GuiUnit, Core::LibraryUnit>;

}
