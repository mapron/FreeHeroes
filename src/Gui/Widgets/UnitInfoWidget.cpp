/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "UnitInfoWidget.hpp"

// Gui
#include "CustomFrames.hpp"
#include "UnitAnimatedPortrait.hpp"
#include "RngParamLabel.hpp"
#include "HoverHelper.hpp"
#include "FormatUtils.hpp"
#include "GeneralPopupDialog.hpp"
#include "LibraryModels.hpp"
#include "UiCommonModel.hpp"
#include "DialogUtils.hpp"

// Core
#include "LibraryFaction.hpp"
#include "AdventureStack.hpp"
#include "BattleStack.hpp"

#include <QBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QVariant>
#include <QIcon>
#include <QPushButton>
#include <QMessageBox>
#include <QPainter>
#include <QEvent>

namespace FreeHeroes::Gui {
using namespace Core;

namespace {

QString toString(const Core::BonusRatio& bonus, bool plus = true, int digits = 0)
{
    const double value = double(bonus.num() * 100) / bonus.denom();
    QString      str   = QString("%1%").arg(value, 0, 'f', digits);
    if (plus && value > 0.) {
        str = "+" + str;
    }
    return str;
}

}

struct UnitInfoWidget::Impl {
    bool                         showModal;
    DarkFrameLabel*              hoverTooltip;
    HoverHelper*                 hoverHelper;
    const LibraryModelsProvider* modelsProvider;
};
class EffectsButton : public QLabel {
public:
    const QSize buttonSize{ 48, 36 };

    EffectsButton(QWidget* parent)
        : QLabel(parent)
    {
        setFrameStyle(QFrame::NoFrame);
        setLineWidth(0);
        setFixedSize(buttonSize);
    }

    void setEffect(QPixmap effectIcon, int duration)
    {
        QImage img(buttonSize, QImage::Format_RGBA8888);
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.drawPixmap(0, 0, effectIcon);

        QFont f = this->font();
        f.setPixelSize(10);
        f.setStyleStrategy(QFont::StyleStrategy(f.styleStrategy() | QFont::NoSubpixelAntialias));
        p.setFont(f);

        QString valueStr = QString::number(duration);

        QFontMetrics fm(f);
        const int    textWidth = fm.horizontalAdvance(valueStr);
        //const int textHeight = fm.height();
        p.setFont(f);

        const int textX = img.width() - textWidth - 2;
        const int textY = img.height() - 2;

        p.setBrush(Qt::black);
        p.setPen(Qt::white);
        p.drawText(QPoint{ textX, textY }, valueStr);
        auto pix = QPixmap::fromImage(img);
        setPixmap(pix);
    }
};

class EffectsListWidget : public QWidget {
public:
    EffectsListWidget(SpellsModel&                         spellsModel,
                      const Core::BattleStack::EffectList& effects,
                      QWidget*                             parent)
        : QWidget(parent)
    {
        QGridLayout* layout = new QGridLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(1);
        for (size_t i = 0; i < effects.size(); ++i) {
            int   row = i / 3;
            int   col = i % 3;
            auto* btn = new EffectsButton(this);
            btn->setEffect(spellsModel.find(effects[i].power.spell)->getIconInt(), effects[i].roundsRemain);
            layout->addWidget(btn, row, col);
        }
    }
};

UnitInfoWidget::UnitInfoWidget(Core::BattleStackConstPtr    battle,
                               Core::AdventureStackConstPtr adventure,
                               const LibraryModelsProvider* modelsProvider,
                               bool                         showModal,
                               QWidget*                     parent)
    : QDialog(parent)
    , m_impl(std::make_unique<Impl>())
{
    setWindowFlag(Qt::FramelessWindowHint, true);
    m_impl->showModal      = showModal;
    m_impl->hoverHelper    = new HoverHelper(modelsProvider, this);
    m_impl->modelsProvider = modelsProvider;

    QFrame*      outerFrame       = new QFrame(this);
    QVBoxLayout* outerLayoutProxy = new QVBoxLayout(this);
    outerLayoutProxy->setContentsMargins(0, 0, 0, 0);
    outerLayoutProxy->setSpacing(0);
    outerLayoutProxy->addWidget(outerFrame);
    outerFrame->setFrameStyle(QFrame::Box);
    outerFrame->setLineWidth(5);
    outerFrame->setProperty("borderStyle", "main");

    QVBoxLayout* mainLayoutProxy = new QVBoxLayout(outerFrame);
    mainLayoutProxy->setContentsMargins(0, 0, 0, 0);
    mainLayoutProxy->setSpacing(0);
    QFrame* innerFrame = new QFrame(this);
    innerFrame->setFrameStyle(QFrame::Box);
    innerFrame->setProperty("borderStyle", "wide");
    innerFrame->setLineWidth(6);
    innerFrame->setProperty("leftBottomFix", 3);
    innerFrame->setProperty("leftTopFix", 3);
    innerFrame->setProperty("specialColor", QColor(230, 0, 0, 128)); // @todo: specalColors.

    // descLabelFrame->setProperty("borderStyle", "common2");
    mainLayoutProxy->addWidget(innerFrame);

    QVBoxLayout* mainLayout = new QVBoxLayout(innerFrame);
    // mainLayout->setMargin(6);
    auto      guiUnit          = modelsProvider->units()->find(adventure->library);
    auto      guiFaction       = modelsProvider->factions()->find(adventure->library->faction);
    const int count            = battle ? battle->count : adventure->count;
    QString   title            = battle ? QString("%1/%2 ").arg(count).arg(adventure->count) + guiUnit->getName(count) : guiUnit->getNameWithCount(count);
    title                      = QString("<font color=\"#EDD57A\">%1</font>").arg(title);
    DarkFrameLabel* titleLabel = new DarkFrameLabel(title, this);
    QFont           f          = this->font();
    f.setPointSize(f.pointSize() + 2);
    titleLabel->setFont(f);
    titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    mainLayout->addWidget(titleLabel);

    QHBoxLayout* paramAndPortraitLayout = new QHBoxLayout();
    paramAndPortraitLayout->setSpacing(3);
    paramAndPortraitLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(paramAndPortraitLayout);

    auto sprite = guiUnit->getBattleSprite();
    Q_ASSERT(sprite);

    auto spriteBk = guiFaction->getUnitBackground();
    Q_ASSERT(spriteBk);

    UnitAnimatedPortrait* portrait = new UnitAnimatedPortrait(sprite, spriteBk, count, showModal, this);
    if (showModal)
        portrait->startTimer();
    QVBoxLayout* portraitWrap = new QVBoxLayout();
    portraitWrap->setContentsMargins(0, 0, 0, 0);
    portraitWrap->setSpacing(0);
    portraitWrap->addWidget(portrait);
    portraitWrap->addStretch();
    paramAndPortraitLayout->addLayout(portraitWrap);

    QVBoxLayout* paramLayout = new QVBoxLayout();
    paramLayout->setSpacing(1);
    paramAndPortraitLayout->addLayout(paramLayout);

    struct Row {
        QString title;
        QString value;
        QString icon;
    };
    const auto& libraryPrimary = adventure->library->primary;
    const auto& advPrimary     = adventure->estimated.primary;
    const auto& battlePrimary  = battle ? battle->current.primary : advPrimary;
    QList<Row>  params;
    params << Row{ tr("Attack"),
                   FormatUtils::formatSequenceInt(libraryPrimary.ad.attack, advPrimary.ad.attack, battlePrimary.ad.attack),
                   "attack" };
    params << Row{ tr("Defense"),
                   FormatUtils::formatSequenceInt(libraryPrimary.ad.defense, advPrimary.ad.defense, battlePrimary.ad.defense),
                   "defense" };
    if (battle && battle->library->traits.rangeAttack)
        params << Row{ tr("Shoots", "Count"),
                       QString::number(battle->remainingShoots) + " / " + QString::number(battlePrimary.shoots),
                       "shoots" };
    params << Row{ tr("Damage"),
                   FormatUtils::formatSequenceDmg(libraryPrimary.dmg, advPrimary.dmg, battlePrimary.dmg),
                   "damage" };

    params << Row{ battle ? tr("Health") : tr("Max Health"), (battle ? QString::number(battle->health) + " / " : "") + FormatUtils::formatSequenceInt(libraryPrimary.maxHealth, advPrimary.maxHealth, battlePrimary.maxHealth), "hp" };

    params << Row{ tr("Speed"),
                   FormatUtils::formatSequenceInt(libraryPrimary.battleSpeed, advPrimary.battleSpeed, battlePrimary.battleSpeed),
                   "speed" };
    if (battle)
        params << Row{ tr("Order on same spd."), "#" + QString::number(battle->sameSpeedOrder + 1), {} };

    for (auto row : params) {
        DarkFrame*   titleValue     = new DarkFrame(this);
        QHBoxLayout* paramLayoutRow = new QHBoxLayout();
        paramLayoutRow->setSpacing(0);
        paramLayoutRow->setContentsMargins(0, 0, 0, 0);

        paramLayout->addLayout(paramLayoutRow);

        QHBoxLayout* titleValueLayout = new QHBoxLayout(titleValue);
        //titleValue->setContentsMargins(5, 5, 5, 5);
        titleValueLayout->setContentsMargins(0, 0, 0, 0);
        titleValueLayout->setSpacing(0);
        auto lbl1 = new QLabel(row.title, this);
        titleValueLayout->addWidget(lbl1, 1, Qt::AlignLeft);
        lbl1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        //lbl1->setMinimumWidth(100);
        auto lbl2 = new QLabel(row.value, this);
        titleValue->setMinimumWidth(180);
        lbl1->setContentsMargins(5, 3, 5, 3);
        lbl2->setContentsMargins(5, 3, 5, 3);
        titleValueLayout->addWidget(lbl2, 0, Qt::AlignRight);
        //titleValueLayout->setStretch(0, 1);
        if (!row.icon.isEmpty()) {
            auto  l   = new DarkFrameLabelIcon(this);
            QIcon ico = QIcon(QPixmap::fromImage(QImage(QString(":/Battle/%1.png").arg(row.icon))));
            QSize s(20, 20);
            l->setPixmap(ico.pixmap(s));
            paramLayoutRow->addWidget(l, Qt::AlignRight);
        } else {
            paramLayoutRow->addSpacing(24);
        }
        paramLayoutRow->addWidget(titleValue, 1, Qt::AlignLeft);
    }
    QHBoxLayout* layoutMoralAndControl = new QHBoxLayout();
    mainLayout->addLayout(layoutMoralAndControl);
    layoutMoralAndControl->setContentsMargins(0, 0, 0, 0);
    layoutMoralAndControl->setSpacing(2);
    {
        const auto rng             = battle ? battle->current.rngParams : adventure->estimated.rngParams;
        auto*      labelMoraleIcon = new MoraleLabel(modelsProvider, this);
        auto*      labelLuckIcon   = new LuckLabel(modelsProvider, this);
        labelMoraleIcon->setValue(rng.morale);
        labelLuckIcon->setValue(rng.luck);
        labelLuckIcon->setDetails(adventure->estimated.luckDetails);
        labelMoraleIcon->setDetails(adventure->estimated.moraleDetails);

        for (auto* lbl : { (QLabel*) labelLuckIcon, (QLabel*) labelMoraleIcon }) {
            m_impl->hoverHelper->addWidgets({ lbl });
            lbl->setProperty("popupOffset", QPoint(300, 200));
            lbl->setProperty("popupOffsetAnchorVert", "center");
        }
        {
            QVBoxLayout* topAligner = new QVBoxLayout();
            topAligner->setContentsMargins(0, 0, 0, 0);
            topAligner->setSpacing(0);
            layoutMoralAndControl->addLayout(topAligner);
            QHBoxLayout* topAlignerH = new QHBoxLayout();
            topAlignerH->addWidget(labelMoraleIcon);
            topAlignerH->addWidget(labelLuckIcon);
            topAligner->addLayout(topAlignerH);
            topAligner->addStretch(1);
        }
        layoutMoralAndControl->addStretch(1);
        if (showModal) {
            FlatButton* btn = new FlatButton(this);
            btn->setProperty("hoverName", tr("Disband squad"));
            m_impl->hoverHelper->addWidgets({ btn });
            auto pixmap = modelsProvider->ui()->disbandStack->get();
            btn->setIcon(pixmap);
            btn->setFixedSize(pixmap.size());
            layoutMoralAndControl->addWidget(btn);
            connect(btn, &QPushButton::clicked, this, [this, modelsProvider, adventure] {
                if (GeneralPopupDialog::confirmRequest(modelsProvider, tr("Are you sure to disband this squad?"), this)) {
                    emit deleteStack(adventure);
                    this->close();
                }
            });
        }
        if (battle) {
            layoutMoralAndControl->addWidget(new EffectsListWidget(*modelsProvider->spells(), battle->appliedEffects, this));
        }
    }

    auto abilsText = abilitiesText(adventure->library);
    abilsText << abilitiesTextExtra(adventure);
    abilsText << retaliationsDescription(battle ? battle->current.maxRetaliations : adventure->library->abilities.maxRetaliations);
    auto resistInfo = this->resistInfo(battle ? battle->current.magicReduce : adventure->estimated.magicReduce,
                                       battle ? battle->current.magicOppSuccessChance : adventure->estimated.magicOppSuccessChance);
    resistInfo += this->immuneInfo(battle ? battle->current.immunities.general : adventure->estimated.immunities.general);
    resistInfo += this->vulnerabilityInfo(adventure->library);

    auto abilsTextStr = abilsText.join(". ");
    if (!resistInfo.isEmpty())
        abilsTextStr += "<br><br>" + resistInfo.join(". ");

    auto castsInfo = this->castsInfo(battle, adventure);
    if (!castsInfo.isEmpty())
        abilsTextStr += "<br><br>" + castsInfo.join(". ");

    QFrame* descLabelFrame = new QFrame(this);
    descLabelFrame->setFrameStyle(QFrame::Panel);
    descLabelFrame->setProperty("borderStyle", "commonDark");
    descLabelFrame->setProperty("fill", true);
    QVBoxLayout* descLabelFrameLayout = new QVBoxLayout(descLabelFrame);
    descLabelFrameLayout->setContentsMargins(5, 5, 5, 5);

    QLabel* descLabel = new QLabel(abilsTextStr, this);
    descLabel->setWordWrap(true);
    descLabel->setMinimumHeight(descLabel->sizeHint().height());
    descLabelFrameLayout->addWidget(descLabel);

    if (showModal) {
        QHBoxLayout* bottom  = new QHBoxLayout();
        QVBoxLayout* bottom2 = new QVBoxLayout();
        mainLayout->addLayout(bottom);
        bottom->addWidget(descLabelFrame);
        bottom->addLayout(bottom2);

        FlatButton* btn = DialogUtils::makeAcceptButton(modelsProvider, this);
        m_impl->hoverHelper->addWidgets({ btn });
        layoutMoralAndControl->addWidget(btn);

        bottom2->addWidget(btn);
        bottom2->addStretch(1);

    } else {
        mainLayout->addWidget(descLabelFrame);
    }
    m_impl->hoverTooltip = new DarkFrameLabel(this);
    m_impl->hoverTooltip->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_impl->hoverTooltip->setContentsMargins(2, 2, 2, 2);
    m_impl->hoverHelper->setHoverLabel(m_impl->hoverTooltip);
    mainLayoutProxy->addWidget(m_impl->hoverTooltip);
}

QStringList UnitInfoWidget::abilitiesText(Core::LibraryUnitConstPtr unit) const
{
    QStringList parts;
    const auto& a = unit->abilities;
    const auto& t = unit->traits;

    if (a.type == Core::UnitType::Living)
        parts << tr("Living");
    if (a.nonLivingType == Core::UnitNonLivingType::Undead)
        parts << tr("Undead");
    if (a.nonLivingType == Core::UnitNonLivingType::Golem)
        parts << tr("Golem");
    if (a.nonLivingType == Core::UnitNonLivingType::Gargoyle)
        parts << tr("Gargoyle");
    if (a.nonLivingType == Core::UnitNonLivingType::Elemental)
        parts << tr("Elemental");
    if (a.nonLivingType == Core::UnitNonLivingType::BattleMachine)
        parts << tr("Battle machine");

    if (t.fly)
        parts << tr("Flies");
    if (t.large && 0)
        parts << tr("Large creature");
    if (t.rangeAttack)
        parts << tr("Shoots");
    if (t.teleport)
        parts << tr("Teleportation");
    if (t.doubleAttack)
        parts << tr("Double attack");
    if (t.freeAttack)
        parts << tr("Target no retaliation");
    if (t.canBeCatapult)
        parts << tr("Attacks walls");
    if (t.returnAfterAttack)
        parts << tr("Return after attack");

    if (a.minimalLuckLevel > 0)
        parts << tr("Always positive luck");
    if (a.minimalMoraleLevel > 0)
        parts << tr("Always positive morale");

    if (a.squadBonus.luck)
        parts << tr("+%1 to army luck").arg(a.squadBonus.luck);
    if (a.squadBonus.morale)
        parts << tr("+%1 to army morale").arg(a.squadBonus.morale);
    if (a.squadBonus.manaCost)
        parts << tr("Reduces spell cost %1 mana").arg(a.squadBonus.manaCost);
    if (a.squadBonus.rngMult.luckPositive != BonusRatio{ 1, 1 })
        parts << tr("Increases luck chance %1").arg(toString(a.squadBonus.rngMult.luckPositive - BonusRatio{ 1, 1 }, true));
    if (a.opponentBonus.luck)
        parts << tr("%1 to enemy luck").arg(a.opponentBonus.luck);
    if (a.opponentBonus.morale)
        parts << tr("%1 to enemy morale").arg(a.opponentBonus.morale);
    if (a.opponentBonus.manaCost)
        parts << tr("Increases enemy spell cost +%1 mana").arg(a.opponentBonus.manaCost);

    if (a.reduceTargetDefense != BonusRatio{ 1, 1 })
        parts << tr("Ignores %1 of target defense").arg(toString(BonusRatio{ 1, 1 } - a.reduceTargetDefense, false));
    if (a.reduceAttackerAttack != BonusRatio{ 1, 1 })
        parts << tr("Ignores %1 of attacker's attack").arg(toString(BonusRatio{ 1, 1 } - a.reduceAttackerAttack, false));

    if (!a.extraDamage.isEmpty()) {
        QStringList enemies;
        for (auto enemy : a.extraDamage.enemies)
            enemies << m_impl->modelsProvider->units()->find(enemy)->getName();
        parts << tr("Extra damage (%1) against (%2)").arg(toString(a.extraDamage.damageBonus)).arg(enemies.join(", "));
    }

    return parts;
}

QStringList UnitInfoWidget::abilitiesTextExtra(AdventureStackConstPtr adventure) const
{
    QStringList parts;
    if (adventure->estimated.regenerate)
        parts << tr("Regenerates health");
    auto& noPenalty = adventure->estimated.disabledPenalties;
    if (adventure->library->traits.rangeAttack) {
        if (noPenalty.contains(RangeAttackPenalty::Distance))
            parts << tr("No distance penalty");
        if (noPenalty.contains(RangeAttackPenalty::Melee))
            parts << tr("No melee penalty");
        if (noPenalty.contains(RangeAttackPenalty::Obstacle))
            parts << tr("No penalty through walls");
        if (noPenalty.contains(RangeAttackPenalty::Blocked))
            parts << tr("Range attack cannot be blocked");
    }
    return parts;
}

QStringList UnitInfoWidget::retaliationsDescription(int countMax) const
{
    QStringList parts;
    if (countMax > 1)
        parts << tr("Retaliate %1 attacks").arg(countMax);
    if (countMax == -1)
        parts << tr("Retaliate all attacks");
    return parts;
}

QStringList UnitInfoWidget::resistInfo(const Core::MagicReduce& reduce, const Core::BonusRatio& successRate) const
{
    QStringList parts;

    if (!reduce.isDefault()) {
        if (reduce.isAllSchoolsEqual()) {
            parts << tr("Damage from any magic:") + toString(reduce.getReduceForSpell(Core::MagicSchool::Air), false);
        } else {
            parts << tr("Damage from air:") + toString(reduce.getReduceForSpell(Core::MagicSchool::Air), false);
            parts << tr("Damage from earth:") + toString(reduce.getReduceForSpell(Core::MagicSchool::Earth), false);
            parts << tr("Damage from fire:") + toString(reduce.getReduceForSpell(Core::MagicSchool::Fire), false);
            parts << tr("Damage from water:") + toString(reduce.getReduceForSpell(Core::MagicSchool::Water), false);
        }
    }
    if (successRate != Core::BonusRatio{ 1, 1 }) {
        parts << tr("Chance to resist spell:") + toString(Core::BonusRatio(1, 1) - successRate);
    }
    return parts;
}

QStringList UnitInfoWidget::immuneInfo(const Core::SpellFilter& immunes) const
{
    if (immunes.isDefault())
        return {};
    QStringList   parts;
    const QString magicLevelsFormat = tr("Immune to magic levels %1");
    if (immunes.containsAll()) {
        parts << tr("Immune to all magic");
        return parts;
    } else if (immunes.levels == std::vector{ 1, 2, 3, 4 }) {
        parts << magicLevelsFormat.arg("1-4");
    } else if (immunes.levels == std::vector{ 1, 2, 3 }) {
        parts << magicLevelsFormat.arg("1-3");
    } else if (!immunes.levels.empty()) {
        QStringList levelsStr;
        for (auto l : immunes.levels)
            levelsStr << QString::number(l);
        parts << magicLevelsFormat.arg(levelsStr.join(","));
    }
    for (auto tag : immunes.tags) {
        if (tag == Core::LibrarySpell::Tag::Ice)
            parts << tr("Immune to Ice");
        else if (tag == Core::LibrarySpell::Tag::Lightning)
            parts << tr("Immune to Lightnings");
        else if (tag == Core::LibrarySpell::Tag::Mind)
            parts << tr("Immune to mind control spells");
        else if (tag == Core::LibrarySpell::Tag::Vision)
            parts << tr("Blind, immune to spells targeting vision");
        else if (tag == Core::LibrarySpell::Tag::AirElem)
            parts << tr("Immune to Lightnings and Armageddon");
        else if (tag == Core::LibrarySpell::Tag::FireElem)
            parts << tr("Immune to pure Fire spells");
    }
    for (auto school : immunes.schools) {
        if (school == Core::MagicSchool::Air)
            parts << tr("Immune to Air magic");
        else if (school == Core::MagicSchool::Earth)
            parts << tr("Immune to Earth magic");
        else if (school == Core::MagicSchool::Fire)
            parts << tr("Immune to Fire magic");
        else if (school == Core::MagicSchool::Water)
            parts << tr("Immune to Water magic");
    }

    for (auto spell : immunes.onlySpells) {
        auto name = m_impl->modelsProvider->spells()->find(spell)->getName();
        parts << tr("Immune to '%1' spell").arg(name);
    }

    return parts;
}

QStringList UnitInfoWidget::vulnerabilityInfo(Core::LibraryUnitConstPtr unit) const
{
    QStringList parts;
    const auto& vulnerable = unit->abilities.vulnerable;
    for (auto tag : vulnerable.tags) {
        if (tag == Core::LibrarySpell::Tag::Ice)
            parts << tr("Vulnerable to Ice");
        else if (tag == Core::LibrarySpell::Tag::Lightning)
            parts << tr("Vulnerable to Lightnings");
        else if (tag == Core::LibrarySpell::Tag::Mind)
            parts << tr("Vulnerable to mind control spells");
        else if (tag == Core::LibrarySpell::Tag::Vision)
            parts << tr("Vulnerable to spells targeting vision");
        else if (tag == Core::LibrarySpell::Tag::AirElem)
            parts << tr("Vulnerable to Lightnings and Armageddon");
        else if (tag == Core::LibrarySpell::Tag::FireElem)
            parts << tr("Vulnerable to pure Fire spells");
    }
    for (auto spell : vulnerable.onlySpells) {
        auto name = m_impl->modelsProvider->spells()->find(spell)->getName();
        parts << tr("Vulnerable to '%1' spell").arg(name);
    }
    using AWE = Core::LibraryUnit::Abilities::AttackWithElement;
    // clang-format off
    switch (unit->abilities.vulnerableAgainstElement) {
        case AWE::Air  : parts << tr("Vulnerable to air charges");   break;
        case AWE::Earth: parts << tr("Vulnerable to earth charges"); break;
        case AWE::Fire : parts << tr("Vulnerable to fire charges");  break;
        case AWE::Ice  : parts << tr("Vulnerable to ice charges");   break;
        default:
        break;
    }
    switch (unit->abilities.attackWithElement) {
        case AWE::Air  : parts << tr("Attack charged with air");   break;
        case AWE::Earth: parts << tr("Attack charged with earth"); break;
        case AWE::Fire : parts << tr("Attack charged with fire");  break;
        case AWE::Ice  : parts << tr("Attack charged with ice");   break;
        case AWE::Mind : parts << tr("Attack charged against mind");   break;
        case AWE::Magic: parts << tr("Weak attack against magic immune");break;
        default:
        break;
    }
    // clang-format on

    return parts;
}

QStringList UnitInfoWidget::castsInfo(BattleStackConstPtr battle, AdventureStackConstPtr adventure) const
{
    QStringList parts;
    const auto& onHitCasts = battle ? battle->current.castsOnHit : adventure->estimated.castsOnHit;
    for (const auto& cast : onHitCasts) {
        auto spellName = m_impl->modelsProvider->spells()->find(cast.params.spell)->getName();
        if (cast.chance == BonusRatio{ 1, 1 })
            parts << tr("Casting '%1' on hit").arg(spellName);
        else
            parts << tr("Casting '%1' on hit with %2 chance").arg(spellName).arg(toString(cast.chance, false));
    }
    const auto& fixedCasts = battle ? battle->current.fixedCast : adventure->estimated.fixedCast;
    if (fixedCasts.count) {
        auto spellName = m_impl->modelsProvider->spells()->find(fixedCasts.params.spell)->getName();
        parts << tr("Casting '%1' (%2)")
                     .arg(spellName)
                     .arg(tr("%1 times", "", fixedCasts.count).arg(fixedCasts.count));
    }
    return parts;
}

UnitInfoWidget::~UnitInfoWidget() = default;

bool UnitInfoWidget::isShowModal() const
{
    return m_impl->showModal;
}

}
