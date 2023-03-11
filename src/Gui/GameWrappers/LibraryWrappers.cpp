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
#include <QVariant>

namespace FreeHeroes::Gui {

namespace {

// Chinese
// 0 - singular/plural
int pluralSelector_CN(int n)
{
    return 0;
}

// English
// 0 - singular, 1 - plural
// default is plural.
int pluralSelector_EN(int n)
{
    assert(n >= -1);
    if (n == -1) // default
        return 1;

    if (n == 1)
        return 0;
    return 1;
}

// Polish
// 0 - singular, 1 - plural, [2 - 2..4 ] [3 - 5+ ]
// default is plural.
int pluralSelector_PL(int n)
{
    assert(n >= -1);
    if (n == -1) // default
        return 1;
    if (n == 1)
        return 0;
    if ((n % 10) >= 2 && (n % 10) <= 4 && ((n % 100 < 10) || (n % 100 > 20)))
        return 2;

    return 3; // see comment for pluralSelector_CYR.
}

// Ukranian/Russian
// 0 - singular, 1 - plural, [2 - 2..4 ] [3 - 5+ ]
// default is plural.
// example.
// 0, singular, "Магог", "Титан"
// 1, abstract plural, "Магоги", "Титаны"
// 2, dual form "Магога", "Титана"
// 3, true plural form "Магогов", "Титанов"
// usage of abstract plural form near the number sounds bad, like '2 Титаны'.
// Sadly, original game does not provide those translations, so we rely on what FH project ships first, then fallback to abstract plural form.
int pluralSelector_CYR(int n)
{
    assert(n >= -1);
    if (n == -1) // default
        return 1;
    if (n % 10 == 1 && n % 100 != 11)
        return 0;

    if ((n % 10) >= 2 && (n % 10) <= 4 && ((n % 100 < 10) || (n % 100 > 20)))
        return 2;

    return 3;
}

// I wish I could reuse plural forms logic from Qt. Sadly, all those rules are hardcoded into 'lrelease' tool and not accessible.
int pluralSelector(int n)
{
    const auto currentLocale = QCoreApplication::instance()->property("currentLocale").toString();
    if (currentLocale == "zh_CN")
        return pluralSelector_CN(n);
    if (currentLocale == "ru_RU" || currentLocale == "uk_UA")
        return pluralSelector_CYR(n);
    if (currentLocale == "pl_PL")
        return pluralSelector_PL(n);

    return pluralSelector_EN(n);
}

bool isValidTranslation(const Core::TranslationMap& tsMap)
{
    const auto currentLocale = QCoreApplication::instance()->property("currentLocale").toString().toStdString();
    if (tsMap.ts.contains(currentLocale)) {
        return !tsMap.ts.at(currentLocale).empty();
    }
    return false;
}

QString translateHelper(const Core::TranslationMap& tsMap,
                        const std::string&          untranslatedName,
                        const std::string&          key,
                        int                         n = -1)
{
    const auto currentLocale = QCoreApplication::instance()->property("currentLocale").toString().toStdString();
    if (tsMap.ts.contains(currentLocale)) {
        return QString::fromStdString(tsMap.ts.at(currentLocale));
    }
    // en_US is default fallback locale.
    if (tsMap.ts.contains("en_US")) {
        return QString::fromStdString(tsMap.ts.at("en_US"));
    }
    // try untranslatedName, if it is empty too - create placeholder from object id. id can't be empty.
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
    QString localizedDesc = translateHelper(getSource()->presentationParams.descr,
                                            getSource()->untranslatedName,
                                            getSource()->id);
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
        // Q_ASSERT(m_battleSprite->get());
    }
    if (!source->presentationParams.spriteProjectile.empty() && !m_projectileSprite->exists()) {
        m_projectileSprite = graphicsLibrary->getObjectAnimation("stub_projectile");
        Q_ASSERT(m_projectileSprite->exists());
    }
}

QString GuiUnit::getName(int n) const
{
    QString localizedName = getNameWithCountImpl(n);
    return localizedName;
}

QString GuiUnit::getNameWithCount(int n, GuiUnit::Variation variation) const
{
    QString localizedName = getNameWithCountImpl(n, variation);
    if (n > 0)
        return tr("%1 %2").arg(n).arg(localizedName);

    return localizedName;
}

QString GuiUnit::getNameWithCountImpl(int n, Variation variation) const
{
    const int                   pluralForm = pluralSelector(n);
    auto&                       p          = getSource()->presentationParams;
    const Core::TranslationMap* tsMap      = &p.name;
    if (pluralForm > 0 && isValidTranslation(p.namePlural))
        tsMap = &p.namePlural;
    if (pluralForm == 2 && isValidTranslation(p.namePluralExt))
        tsMap = &p.namePluralExt;
    if (pluralForm == 3 && isValidTranslation(p.namePluralExt2))
        tsMap = &p.namePluralExt2;

    if (variation == Variation::AsTarget) {
        if (pluralForm == 0 && isValidTranslation(p.nameAsTarget))
            tsMap = &p.nameAsTarget;
        if (pluralForm > 0 && isValidTranslation(p.nameAsTargetPlural))
            tsMap = &p.nameAsTargetPlural;
    }
    QString localizedName = translateHelper(*tsMap, getSource()->untranslatedName, getSource()->id, n);
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
    if (level < 0 || level > 2)
        return {};
    auto&                                    p = getSource()->presentationParams;
    std::vector<const Core::TranslationMap*> maps{ &p.descrBasic, &p.descrAdvanced, &p.descrExpert };

    QString localizedDesc = translateHelper(*maps.at(level),
                                            getSource()->untranslatedName,
                                            getSource()->id);
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
    auto&                                    p = getSource()->presentationParams;
    std::vector<const Core::TranslationMap*> maps{ &p.descrNormal, &p.descrBasic, &p.descrAdvanced, &p.descrExpert };

    QString localizedDesc = translateHelper(*maps.at(level),
                                            getSource()->untranslatedName,
                                            getSource()->id);
    localizedDesc         = prepareDescription(localizedDesc);
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
