/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "HeroParameterWidget.hpp"

#include "DependencyInjector.hpp"
#include "LibraryModels.hpp"
#include "UiCommonModel.hpp"
#include "FormatUtils.hpp"

// Core
#include "AdventureSquad.hpp"
#include "AdventureHero.hpp"

#include "MernelPlatform/Profiler.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>

#include <functional>

namespace FreeHeroes::Gui {
namespace {
const QString formatTitle = QString("<font color=\"#EDD57A\">%1</font>");
const QString formatHdr   = QString("<font color=\"#9E9986\">%1</font>");

using FormatterCallback = std::function<QString(Core::AdventureHeroConstPtr hero)>;
using ZeroCallback      = std::function<void()>;
struct Record {
    QString           title;
    FormatterCallback makeValue;
};

QString toString(int value)
{
    return QString("%1").arg(value);
}

QString toString(const Core::BonusRatio& bonus, bool plus = true, int digits = 0)
{
    return FormatUtils::formatBonus(bonus, plus, digits);
}

}

struct HeroParameterWidget::Impl {
    Core::AdventureHeroConstPtr  m_hero = nullptr;
    const LibraryModelsProvider* m_modelsProvider;
    UiCommonModel*               m_ui = nullptr;
    QList<ZeroCallback>          m_onRefresh;
};

HeroParameterWidget::HeroParameterWidget(const LibraryModelsProvider* modelProvider, QWidget* parent)
    : QWidget(parent)
    , m_impl(std::make_unique<Impl>())
{
    m_impl->m_modelsProvider = modelProvider;
    m_impl->m_ui             = modelProvider->ui();

    Mernel::ProfilerScope scope("HeroParameterWidget");
    QHBoxLayout*          proxylayout = new QHBoxLayout(this);
    proxylayout->setContentsMargins(0, 0, 0, 0);
    proxylayout->setSpacing(0);
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    proxylayout->addWidget(scrollArea);
    // proxylayout->addStretch();
    QWidget* proxyWidget = new QWidget(this);
    scrollArea->setWidget(proxyWidget);
    QGridLayout* layout = new QGridLayout(proxyWidget);
    layout->setContentsMargins(10, 5, 10, 10);
    layout->setHorizontalSpacing(5);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(3, 1);

    int  row        = 0;
    auto makeHeader = [&row, layout, this](QString title) {
        QFont f = this->font();
        f.setPointSize(f.pointSize() + 3);
        QLabel* lbl = new QLabel(formatHdr.arg(title), this);
        lbl->setFont(f);
        layout->addWidget(lbl, row, 1, 1, 3, Qt::AlignLeft);
        row++;
    };

    auto makeSubHeader = [&row, layout, this](QString title) {
        QFont f = this->font();
        f.setPointSize(f.pointSize());
        QLabel* lbl = new QLabel(formatTitle.arg(title), this);
        lbl->setFont(f);
        layout->addWidget(lbl, row, 1, 1, 3, Qt::AlignLeft);
        row++;
    };

    auto makeRowPlainText = [&row, layout, this](FormatterCallback makeValue) {
        QLabel* lblValue = new QLabel(this);
        layout->addWidget(new QWidget(this), row, 0);

        layout->addWidget(lblValue, row, 1, 1, 2, Qt::AlignLeft);
        layout->addWidget(new QWidget(this), row, 3);
        ZeroCallback updater = [lblValue, makeValue, this] {
            QString formatted = makeValue(this->m_impl->m_hero);
            lblValue->setText(formatted);
        };
        this->m_impl->m_onRefresh << updater;
        row++;
    };

    auto makeRow = [&row, layout, this](const Record& rec) {
        QLabel* lblTitle = new QLabel(formatTitle.arg(rec.title), this);
        QLabel* lblValue = new QLabel(this);
        layout->addWidget(new QWidget(this), row, 0);
        layout->addWidget(lblTitle, row, 1, Qt::AlignLeft);
        layout->addWidget(lblValue, row, 2, Qt::AlignRight);
        layout->addWidget(new QWidget(this), row, 3);
        auto         cb      = rec.makeValue;
        ZeroCallback updater = [lblValue, cb, this] {
            QString formatted = cb(this->m_impl->m_hero);
            lblValue->setText(formatted);
        };
        this->m_impl->m_onRefresh << updater;
        row++;
    };
    auto makeRows = [&makeHeader, &makeRow](QString title, const QList<Record>& records) {
        makeHeader(title);
        for (const auto& rec : records)
            makeRow(rec);
    };
    // ----------------------------
    using PT = Core::HeroPrimaryParamType;

    // clang-format off
    QList<Record> main {
        {tr("Level")       , [](auto hero){ return toString(hero->level                               ); }},
        {tr("Experience")  , [](auto hero){ return toString(hero->experience                          ); }},

        {m_impl->m_ui->skillInfo[PT::Attack      ].name, [](auto hero){ return toString(hero->estimated.primary.ad.attack         ); }},
        {m_impl->m_ui->skillInfo[PT::Defense     ].name, [](auto hero){ return toString(hero->estimated.primary.ad.defense        ); }},
        {m_impl->m_ui->skillInfo[PT::SpellPower  ].name, [](auto hero){ return toString(hero->estimated.primary.magic.spellPower  ); }},
        {m_impl->m_ui->skillInfo[PT::Intelligence].name, [](auto hero){ return toString(hero->estimated.primary.magic.intelligence); }},
    };

    QList<Record> physical {
        {tr("Melee bonus") , [](auto hero){ return toString(hero->estimated.meleeAttack); }},
        {tr("Range bonus") , [](auto hero){ return toString(hero->estimated.rangedAttack); }},
        {tr("Def. reduce") , [](auto hero){ return toString(hero->estimated.defense); }},
    };
    QList<Record> magical {
        {tr("Current mana"       ) , [](auto hero){ return toString(hero->mana); }},
        {tr("Intelligence effect") , [](auto hero){ return toString(hero->estimated.manaIncrease); }},
        {tr("Max mana"           ) , [](auto hero){ return toString(hero->estimated.maxMana); }},
        {tr("Mana regen / day"   ) , [](auto hero){ return toString(hero->estimated.manaRegenAbs); }},
        {tr("Maximum spell level<br>"
            "written in spellbook") , [](auto hero){ return toString(hero->estimated.maxLearningSpellLevel); }},
        {tr("Maximum spell level<br>"
            "to teach another hero") , [](auto hero){ return toString(hero->estimated.maxTeachingSpellLevel); }},
        {tr("Extra rounds for<br>"
            "spell duration"   ) , [](auto hero){ return toString(hero->estimated.extraRounds); }},

        {tr("Resist chance"       ) , [](auto hero){ return toString(hero->estimated.magicResistChance); }},
        {tr("All magic damage"    ) , [](auto hero){ return toString(hero->estimated.magicIncrease.allMagic); }},
        {tr("Air magic damage"      ) , [](auto hero){ return toString(hero->estimated.magicIncrease.air); }},
        {tr("Earth magic damage"    ) , [](auto hero){ return toString(hero->estimated.magicIncrease.earth); }},
        {tr("Fire magic damage"     ) , [](auto hero){ return toString(hero->estimated.magicIncrease.fire); }},
        {tr("Water magic damage"    ) , [](auto hero){ return toString(hero->estimated.magicIncrease.water); }},
    };

    QList<Record> speed {
        {tr("Speed battle bonus"   ) , [](auto hero){ return toString(hero->estimated.unitBattleSpeedAbs); }},
        {tr("Army max battle speed") , [](auto hero){ return toString(hero->estimatedFromSquad.fastestBattleSpeed); }},
        {tr("Army speed"           ) , [](auto hero){ return toString(hero->estimatedFromSquad.armySpeed); }},
        {tr("Army MP"              ) , [](auto hero){ return toString(hero->estimated.armyMovePoints); }},
        {tr("MP bonus"             ) , [](auto hero){ return toString(hero->estimated.mpIncrease); }},
        {tr("Extra MP abs. "       ) , [](auto hero){ return toString(hero->estimated.extraMovePoints); }},
        {tr("Next day MP "         ) , [](auto hero){ return toString(hero->estimated.nextDayMovePoints); }},

        {tr("Water MP bonus"       ) , [](auto hero){ return toString(hero->estimated.mpWaterIncrease); }},
        {tr("Extra Water MP abs. " ) , [](auto hero){ return toString(hero->estimated.extraMovePointsWater); }},
        {tr("Next day Water MP "   ) , [](auto hero){ return toString(hero->estimated.nextDayMovePointsWater); }},
    };
    QList<Record> skills {
        {tr("Scouting radius"                   ) , [](auto hero){ return toString(hero->estimated.scoutingRadius); }},
        {tr("Reduced surrender cost"            ) , [](auto hero){ return "-" + toString(hero->estimated.surrenderDiscount, false); }},
        {tr("Neutral join chance"               ) , [](auto hero){ return toString(hero->estimated.neutralJoinChance, false); }},
        {tr("Minimal level for Great Library"   ) , [](auto hero){ return toString(hero->estimated.greatLibraryVisitLevel); }},
        {tr("Experience gain bonus"             ) , [](auto hero){ return toString(hero->estimated.bonusExperience); }},
        {tr("Eagle Eye chance"                  ) , [](auto hero){ return toString(hero->estimated.eagleEyeChance, false); }},
        {tr("Necromancy efficiency"             ) , [](auto hero){ return toString(hero->estimated.necromancy, false); }},

    };
    // clang-format on

    makeRows(tr("Primary stats"), main);
    makeRows(tr("Physical damage"), physical);
    makeRows(tr("Magical"), magical);
    makeRows(tr("Speed"), speed);

    makeHeader(tr("Army luck"));
    makeRow({ tr("Luck value"), [](auto hero) { return toString(hero->estimatedFromSquad.rngParams.luck); } });
    makeRowPlainText([this](auto hero) { return m_impl->m_ui->getLuckDescription(hero->estimatedFromSquad.luckDetails).join("<br>"); });

    makeHeader(tr("Army morale"));
    makeRow({ tr("Morale value"), [](auto hero) { return toString(hero->estimatedFromSquad.rngParams.morale); } });
    makeRowPlainText([this](auto hero) { return m_impl->m_ui->getMoraleDescription(hero->estimatedFromSquad.moraleDetails).join("<br>"); });

    makeHeader(tr("Skills"));
    for (const auto& rec : skills)
        makeRow(rec);

    auto printSkillsProbabilities = [this](const Core::LibraryFactionHeroClass::SkillWeights& weights) -> QStringList {
        if (weights.empty())
            return {};
        QStringList result;
        int         total = 0;
        for (auto& p : weights)
            total += p.second;
        if (!total)
            return {};
        for (auto& p : weights) {
            auto skillName = m_impl->m_modelsProvider->skills()->find(p.first)->getName();
            auto skillProb = toString(Core::BonusRatio(p.second, total), false);
            result << QString("%1: %2").arg(skillName).arg(skillProb);
        }
        return result;
    };
    makeSubHeader(tr("Upgrade - prioritized"));
    makeRowPlainText([printSkillsProbabilities](auto hero) { return printSkillsProbabilities(hero->estimated.levelUp.weightsForUpgrade.high).join("<br>"); });
    makeSubHeader(tr("Upgrade - normal"));
    makeRowPlainText([printSkillsProbabilities](auto hero) { return printSkillsProbabilities(hero->estimated.levelUp.weightsForUpgrade.normal).join("<br>"); });

    makeSubHeader(tr("New - prioritized"));
    makeRowPlainText([printSkillsProbabilities](auto hero) { return printSkillsProbabilities(hero->estimated.levelUp.weightsForNew.high).join("<br>"); });
    makeSubHeader(tr("New - normal"));
    makeRowPlainText([printSkillsProbabilities](auto hero) { return printSkillsProbabilities(hero->estimated.levelUp.weightsForNew.normal).join("<br>"); });

    makeRow({ tr("Levelups to wisdom suggest"), [](auto hero) { return toString(hero->estimated.levelUp.wisdom.suggestEveryLevel - hero->levelupsWithoutWisdom); } });
    makeRow({ tr("Levelups to magic school suggest"), [](auto hero) { return toString(hero->estimated.levelUp.school.suggestEveryLevel - hero->levelupsWithoutSchool); } });
}

HeroParameterWidget::~HeroParameterWidget() = default;

void HeroParameterWidget::refresh()
{
    if (!m_impl->m_hero)
        return;
    for (auto& cb : m_impl->m_onRefresh)
        cb();
}

void HeroParameterWidget::setSource(Core::AdventureHeroConstPtr hero)
{
    m_impl->m_hero = hero;
}

}
