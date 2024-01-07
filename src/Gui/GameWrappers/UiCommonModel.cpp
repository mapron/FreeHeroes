/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "UiCommonModel.hpp"
#include "FormatUtils.hpp"
#include "IGraphicsLibrary.hpp"
#include "IMusicBox.hpp"

namespace FreeHeroes::Gui {

namespace {

enum class IconSize
{
    Small,
    Medium,
    Large
};
inline IGraphicsLibrary::PixmapKey primaryHeroStatResourceId(Core::HeroPrimaryParamType param, IconSize size)
{
    int         index = static_cast<int>(param);
    std::string base;
    if (size == IconSize::Small) { // 32x32, pskil32,  natural order
        base = "pskil32";
    } else if (size == IconSize::Medium) { // 42x42, pskil42, int is last
        // clang-format off
        switch (param) {
            case Core::HeroPrimaryParamType::Mana:          index = 3; break;
            case Core::HeroPrimaryParamType::Experience:    index = 4; break;
            case Core::HeroPrimaryParamType::Intelligence:  index = 5; break;
            default: break;
        }
        // clang-format on
        base = "pskil42";
    } else if (size == IconSize::Large) { // 82x93, pskill , natural order
        base = "pskill";
    }
    return IGraphicsLibrary::PixmapKey(base, index);
}

}

UiCommonModel::UiCommonModel(Sound::IMusicBox*       musicBox,
                             const IGraphicsLibrary* graphicsLibrary,
                             QObject*                parent)
    : QObject(parent)
{
    auto getTypeTranslation = [](Core::HeroPrimaryParamType type) -> QString {
        // clang-format off
        switch(type) {
            case Core::HeroPrimaryParamType::Attack      : return tr("Attack");
            case Core::HeroPrimaryParamType::Defense     : return tr("Defense");
            case Core::HeroPrimaryParamType::SpellPower  : return tr("Spell Power");
            case Core::HeroPrimaryParamType::Intelligence: return tr("Intelligence");
            case Core::HeroPrimaryParamType::Experience  : return tr("Experience");
            case Core::HeroPrimaryParamType::Mana        : return tr("Mana");
        }
        // clang-format on
        return "";
    };
    QStringList descriptions{

    };
    auto getTypeDescription = [](Core::HeroPrimaryParamType type) -> QString {
        // clang-format off
        switch(type) {
            case Core::HeroPrimaryParamType::Attack      : return tr("{Attack} of hero increases creature damage");
            case Core::HeroPrimaryParamType::Defense     : return tr("{Defense} of hero decreases damage taken by creature");
            case Core::HeroPrimaryParamType::SpellPower  : return tr("{Spell Power} determines duration and efficency of spells");
            case Core::HeroPrimaryParamType::Intelligence: return tr("{Intelligence} determines how many mana hero has.<br>Usually mana is 10 times of {Intelligence}.");
            case Core::HeroPrimaryParamType::Experience  : return tr("{Level}: %1<br><br>{Current experience:} %2<br><br>{Next level:} %3<br><br>{Remain experience:} %4");
            case Core::HeroPrimaryParamType::Mana        : return tr("{Mana}<br><br>Current mana is %1 from %2 possible. <br>Hero can have mana above maximum, but it won't regenerate every day.");
        }
        // clang-format on
        return "";
    };
    for (auto type : {
             Core::HeroPrimaryParamType::Attack,
             Core::HeroPrimaryParamType::Defense,
             Core::HeroPrimaryParamType::SpellPower,
             Core::HeroPrimaryParamType::Intelligence,
             Core::HeroPrimaryParamType::Experience,
             Core::HeroPrimaryParamType::Mana,
         }) {
        skillInfo[type] = PrimarySkillInfo{
            getTypeTranslation(type),
            FormatUtils::prepareDescription(getTypeDescription(type)),
            graphicsLibrary->getPixmapByKey(primaryHeroStatResourceId(type, IconSize::Small)),
            graphicsLibrary->getPixmapByKey(primaryHeroStatResourceId(type, IconSize::Medium)),
            graphicsLibrary->getPixmapByKey(primaryHeroStatResourceId(type, IconSize::Large))
        };
    }

    spellbook.background = graphicsLibrary->getPixmapByKey({ "spelbk2" });

    // In resource file, tabs come not in order...
    std::vector tabIndices{ 0, 3, 1, 2, 4 };
    std::vector schoolsIndices{ 0, 3, 1, 2 };
    for (int i = 0; i < 5; i++) {
        spellbook.spelltabs << graphicsLibrary->getPixmapByKey({ "speltab", 0, tabIndices[i] });
        if (i < 4) {
            spellbook.spellTitles << graphicsLibrary->getPixmapByKey({ "schools", 0, schoolsIndices[i] });
        }
    }
    spellbook.prevPage = graphicsLibrary->getPixmapByKey({ "speltrnl" });
    spellbook.nextPage = graphicsLibrary->getPixmapByKey({ "speltrnr" });

    disbandStack   = graphicsLibrary->getPixmapByKey({ "iviewcr2" });
    buttons.close  = graphicsLibrary->getIcon("hsbtns", 2);
    buttons.okWide = graphicsLibrary->getIcon("iokay", 3);
    buttons.cancel = graphicsLibrary->getIcon("icancel", 3); // really 4, but we don't use fourth

    buttons.scrollLeft  = graphicsLibrary->getIcon("hsbtns3", 3);
    buttons.scrollRight = graphicsLibrary->getIcon("hsbtns5", 3);

    clickEffect = musicBox->effectPrepare({ Sound::IMusicBox::EffectSet::Click });

    heroDialog.distantIcons = graphicsLibrary->getIcon({ { "hsbtns6", 0, 0 }, { "hsbtns6", 0, 3 } });
    heroDialog.compactIcons = graphicsLibrary->getIcon({ { "hsbtns7", 0, 0 }, { "hsbtns7", 0, 3 } });
    heroDialog.tacticsIcons = graphicsLibrary->getIcon("hsbtns8", 3);
    heroDialog.stackSplit   = graphicsLibrary->getIcon("hsbtns9", 3);

    heroDialog.deleteHero = graphicsLibrary->getIcon("hsbtns2", 3);
    heroDialog.bag        = graphicsLibrary->getIcon("bckpck", 2);
    heroDialog.listInfo   = graphicsLibrary->getIcon("hsbtns4", 2);

    heroDialog.labelFlag = graphicsLibrary->getPixmapByKey({ "crest58" });

    for (int i = -3; i <= +3; i++) {
        luck.large[i]  = graphicsLibrary->getPixmapByKey({ "ilck82", 0, i + 3 });
        luck.medium[i] = graphicsLibrary->getPixmapByKey({ "ilck42", 0, i + 3 });
        luck.small[i]  = graphicsLibrary->getPixmapByKey({ "ilck22", 0, i + 3 });

        morale.large[i]  = graphicsLibrary->getPixmapByKey({ "imrl82", 0, i + 3 });
        morale.medium[i] = graphicsLibrary->getPixmapByKey({ "imrl42", 0, i + 3 });
        morale.small[i]  = graphicsLibrary->getPixmapByKey({ "imrl22", 0, i + 3 });
    }

    // losecslp => lose on siege loop;
    // losecstl => lose on siege  start;
    // lbstart => lose start
    // lbloop  => lose loop
    // rtloop  => retreat loop
    // win3 => ok loop
    win.anim  = graphicsLibrary->getVideo("win3");
    lose.anim = graphicsLibrary->getVideo("lbloop");

    win.music  = musicBox->musicPrepare(Sound::IMusicBox::MusicSettings{ Sound::IMusicBox::MusicSet::Win }.setLoopAround(false));
    lose.music = musicBox->musicPrepare(Sound::IMusicBox::MusicSettings{ Sound::IMusicBox::MusicSet::Lose }.setLoopAround(false));

    battleControl.wait       = graphicsLibrary->getIconFromCodeList({ "icm0060", "icm0061", "icm0062" });
    battleControl.guard      = graphicsLibrary->getIconFromCodeList({ "icm0070", "icm0071", "icm0072" });
    battleControl.spellBook  = graphicsLibrary->getIconFromCodeList({ "icm0050", "icm0051", "icm0052" });
    battleControl.surrender  = graphicsLibrary->getIconFromCodeList({ "icm0010", "icm0011", "icm0012" });
    battleControl.autoCombat = graphicsLibrary->getIconFromCodeList({ "icm0040", "icm0041", "icm0042" });
    battleControl.settings   = graphicsLibrary->getIconFromCodeList({ "icm0030", "icm0031", "icm0032" });

    battleControl.unitCast    = graphicsLibrary->getIconFromCodeList({ "icmalt030", "icmalt031", "icmalt032" });
    battleControl.rangeAttack = graphicsLibrary->getIconFromCodeList({ "icmalt020", "icmalt021", "icmalt022" });
    battleControl.meleeAttack = graphicsLibrary->getIconFromCodeList({ "icmalt010", "icmalt011", "icmalt012" });
}

QString UiCommonModel::getCommonString(UiCommonModel::UIString common) const
{
    switch (common) {
        case UIString::Close:
            return tr("Close");
        case UIString::Cancel:
            return tr("Cancel");
    }
    return "";
}

QStringList UiCommonModel::getLuckDescription(const Core::LuckDetails& details) const
{
    QStringList mods;
    int         value = details.total;
    if (details.unitBonus)
        mods << tr("Creatures present who adds luck: +%1").arg(details.unitBonus);
    if (details.skills)
        mods << tr("Luck skill bonus: +%1").arg(details.skills);
    if (details.artifacts)
        mods << tr("Artifacts: +%1").arg(details.artifacts);
    if (details.minimalLuckLevel > 0 && value < details.minimalLuckLevel)
        mods << tr("Creature minimal luck is +%1, increasing total luck from %2 to +%1")
                    .arg(details.minimalLuckLevel)
                    .arg(value);
    if (mods.empty())
        mods << tr("No modificators");

    value = std::max(value, details.minimalLuckLevel);
    if (value > 0) {
        mods << tr("Chance of positive luck: %1").arg(FormatUtils::formatBonus(details.rollChance, false, 1));
        if (details.rollChance.num() == 0)
            mods << tr("Positive luck was neutralized by artifact or terrain");
    } else if (value < 0) {
        mods << tr("Chance of negative luck: %1").arg(FormatUtils::formatBonus(details.rollChance, false, 1));
        if (details.rollChance.num() == 0)
            mods << tr("Negative luck was neutralized by artifact or terrain");
    }

    return mods;
}

QStringList UiCommonModel::getMoraleDescription(const Core::MoraleDetails& details) const
{
    QStringList mods;
    int         value = details.total;
    if (details.undead)
        mods << tr("Army has undead: -1");
    if (details.factionsPenalty == 1)
        mods << tr("All army within same faction: +1");
    else if (details.factionsPenalty < 0)
        mods << tr("Army has %1 different factions: %2").arg(details.extraUnwantedFactions + 1).arg(details.factionsPenalty);
    if (details.unitBonus)
        mods << tr("Creatures present who adds morale: +%1").arg(details.unitBonus);
    if (details.skills)
        mods << tr("Leadership bonus: +%1").arg(details.skills);
    if (details.artifacts)
        mods << tr("Artifacts: +%1").arg(details.artifacts);
    if (details.unaffected)
        mods << tr("Creature is unaffected by morale");
    if (details.minimalMoraleLevel > 0 && value < details.minimalMoraleLevel)
        mods << tr("Creature minimal morale is +%1, increasing total morale from %2 to +%1")
                    .arg(details.minimalMoraleLevel)
                    .arg(value);
    if (mods.empty())
        mods << tr("No modificators");

    value = std::max(value, details.minimalMoraleLevel);
    if (value > 0) {
        mods << tr("Chance of positive morale: %1").arg(FormatUtils::formatBonus(details.rollChance, false, 1));
        if (details.rollChance.num() == 0)
            mods << tr("Positive morale was neutralized by artifact or terrain");
    } else if (value < 0) {
        mods << tr("Chance of negative morale: %1").arg(FormatUtils::formatBonus(details.rollChance, false, 1));
        if (details.rollChance.num() == 0)
            mods << tr("Negative morale was neutralized by artifact or terrain");
    }

    return mods;
}

UiCommonModel::~UiCommonModel() = default;

}
