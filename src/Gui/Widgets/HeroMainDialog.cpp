/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "HeroMainDialog.hpp"

#include "ui_HeroMainDialog.h"

// Gui
#include "UnitInfoWidget.hpp"
#include "ResizePixmap.hpp"
#include "SpellBookDialog.hpp"
#include "DialogUtils.hpp"
#include "GeneralPopupDialog.hpp"
#include "HoverHelper.hpp"
#include "FormatUtils.hpp"
#include "AdventureWrappers.hpp"
#include "LibraryModels.hpp"
#include "UiCommonModel.hpp"

// Core
#include "IAdventureControl.hpp"
#include "AdventureArmy.hpp"
#include "LibraryHeroSpec.hpp"

#include <QButtonGroup>

namespace FreeHeroes::Gui {
using namespace Core;

namespace {
const QString formatTwoLine = QString("<p style=\"line-height:70%\"><font color=\"#EDD57A\">%1</font></p><p>%2</p>");
}

HeroMainDialog::HeroMainDialog(const LibraryModelsProvider* modelsProvider, QWidget* parent)
    : QDialog(parent)
    , m_ui(std::make_unique<Ui::HeroMainDialog>())
    , m_modelsProvider(modelsProvider)
{
    setWindowFlag(Qt::FramelessWindowHint, true);
    m_ui->setupUi(this, std::tuple{ modelsProvider });
    m_hoverHelper = std::make_unique<HoverHelper>(modelsProvider, this);

    QFont f = m_ui->labelHeroName->font();
    if (f.pixelSize() != -1)
        f.setPixelSize(f.pixelSize() + 3);
    else
        f.setPointSize(f.pointSize() + 3);
    m_ui->labelHeroName->setFont(f);
    for (auto* label : { m_ui->labelExperienceValue,
                         m_ui->labelManaValue,
                         m_ui->labelSpecialityText,
                         m_ui->labelHeroName }) {
        label->setFrameStyle(QFrame::Panel);
        label->setLineWidth(2);
        label->setProperty("borderStyle", "commonDark");
        label->setProperty("fill", true);
        label->setMargin(4);
        //label->setContentsMargins(5, 2, 5, 3);
    }
    for (auto* label : { m_ui->labelExperienceIcon,
                         m_ui->labelManaIcon }) {
        label->setLineWidth(2);
        label->setFrameStyle(QFrame::Panel);
        label->setProperty("borderStyle", "commonDark");
        label->setContentsMargins(0, 0, 0, 0);
    }
    for (auto* pb : { m_ui->pushButtonClose,
                      m_ui->pushButtonDeleteHero,
                      m_ui->pushButtonBag,
                      m_ui->pushButtonListInfo }) {
        //pb->setIconSize(pb->size() - QSize{4,4});
        m_hoverHelper->addWidgets({ pb });
    }

    m_ui->pushButtonDeleteHero->setProperty("hoverName", tr("Delete hero"));
    m_ui->pushButtonBag->setProperty("hoverName", tr("Artifacts bag"));
    m_ui->pushButtonListInfo->setProperty("hoverName", tr("Hero information"));

    m_hoverHelper->addAlias(m_ui->labelExperienceValue, m_ui->labelExperienceIcon);
    m_hoverHelper->addAlias(m_ui->labelManaValue, m_ui->labelManaIcon);
    m_hoverHelper->addAlias(m_ui->labelHeroName, m_ui->labelHeroPortrait);

    m_ui->labelHeroPortrait->setAttribute(Qt::WA_Hover);
    m_ui->labelHeroName->setAttribute(Qt::WA_Hover);

    m_ui->labelHeroName->setProperty("popupOffset", QPoint(11, 83));

    m_ui->labelExperienceValue->setProperty("hoverName", tr("Information about experience"));
    m_ui->labelExperienceValue->setProperty("popupOffset", QPoint(160, 240));
    m_ui->labelExperienceValue->setProperty("popupOffsetAnchorVert", "center");

    m_ui->labelManaValue->setProperty("hoverName", tr("Information about mana"));
    m_ui->labelManaValue->setProperty("popupOffset", QPoint(310, 240));
    m_ui->labelManaValue->setProperty("popupOffsetAnchorVert", "center");

    for (auto* lbl : { (QLabel*) m_ui->labelMoraleIcon, (QLabel*) m_ui->labelLuckIcon }) {
        m_hoverHelper->addWidgets({ lbl });
        lbl->setProperty("popupOffset", QPoint(310, 240));
        lbl->setProperty("popupOffsetAnchorVert", "center");
    }

    QButtonGroup* formations = new QButtonGroup(this);
    formations->setExclusive(true);
    m_ui->pushButtonUseCompactFormation->setCheckable(true);
    m_ui->pushButtonUseDistantFormation->setCheckable(true);
    formations->addButton(m_ui->pushButtonUseCompactFormation);
    formations->addButton(m_ui->pushButtonUseDistantFormation);
    connect(m_ui->pushButtonUseCompactFormation, &QPushButton::clicked, this, [this]() {
        m_adventureSquadControl->setCompactFormation(m_ui->pushButtonUseCompactFormation->isChecked());
    });
    connect(m_ui->pushButtonUseDistantFormation, &QPushButton::clicked, this, [this]() {
        m_adventureSquadControl->setCompactFormation(m_ui->pushButtonUseCompactFormation->isChecked());
    });

    for (auto* btn : { m_ui->pushButtonUseDistantFormation,
                       m_ui->pushButtonUseCompactFormation,
                       m_ui->pushButtonTacticsDisable,
                       m_ui->pushButtonSplit,
                       m_ui->pushButtonDeleteHero,
                       m_ui->pushButtonBag,
                       m_ui->pushButtonListInfo })
        DialogUtils::setupClickSound(modelsProvider, btn);

    m_ui->pushButtonTacticsDisable->setEnabled(false); // @todo: tactics someday disablement.
    m_ui->pushButtonSplit->setEnabled(false);          // @todo: split

    m_ui->pushButtonUseDistantFormation->setProperty("hoverName", tr("Use distant formation"));
    m_ui->pushButtonUseCompactFormation->setProperty("hoverName", tr("Use compact formation"));
    m_ui->pushButtonTacticsDisable->setProperty("hoverName", tr("Disable tactics formation"));
    m_ui->pushButtonSplit->setProperty("hoverName", tr("Split squad"));

    DialogUtils::makeAcceptButton(modelsProvider, this, m_ui->pushButtonClose, false);
    connect(m_ui->armyControlWidget, &ArmyControlWidget::showInfo, this, &HeroMainDialog::showInfo);
    connect(m_ui->armyControlWidget, &ArmyControlWidget::hideInfo, this, &HeroMainDialog::hideInfo);

    connect(m_ui->pushButtonListInfo, &QPushButton::clicked, this, [this] {
        m_ui->stackedWidgetStats->setCurrentIndex(m_ui->pushButtonListInfo->isChecked() ? 1 : 0);
    });
    connect(m_ui->artifactPuppetWidget, &ArtifactPuppetWidget::openSpellBook, this, &HeroMainDialog::openSpellBook);

    m_hoverHelper->setHoverLabel(m_ui->labelHover);

    m_ui->primaryStatsWidget->setHoverLabel(m_ui->labelHover);
    m_ui->skillsGridWidget->setHoverLabel(m_ui->labelHover);
    m_ui->artifactPuppetWidget->setHoverLabel(m_ui->labelHover);
}

void HeroMainDialog::refresh()
{
    updateHeroAppearence();
    m_ui->armyControlWidget->refresh();
    m_ui->artifactPuppetWidget->refresh();
    m_ui->pageStatsList->refresh();
}

HeroMainDialog::~HeroMainDialog() = default;

void HeroMainDialog::setSource(const GuiAdventureArmy*       heroArmy,
                               Core::IAdventureSquadControl* adventureSquadControl,
                               Core::IAdventureHeroControl*  adventureHeroControl)
{
    m_heroArmy              = heroArmy;
    m_hero                  = m_heroArmy->getHero();
    m_adventureSquadControl = adventureSquadControl;
    m_adventureHeroControl  = adventureHeroControl;

    m_ui->armyControlWidget->setSource(heroArmy->getSquad(), adventureSquadControl);
    m_ui->artifactPuppetWidget->setSource(m_hero, adventureHeroControl);
    m_ui->pageStatsList->setSource(m_hero->getSource());

    m_ui->ftopMain->setProperty("specialColor", QColor(230, 0, 0, 128)); // @todo: player color.
    m_ui->ftopMain->update();

    updateGraphics();
}

void HeroMainDialog::updateHeroAppearence()
{
    const QString heroName  = m_hero->getName();
    const QString heroClass = m_hero->getClassName();
    m_ui->labelHeroName->setText(
        formatTwoLine
            .arg(heroName)
            .arg(tr("Level %1, %2").arg(m_hero->getSource()->level).arg(heroClass)));

    m_ui->labelHeroPortrait->setPixmap(m_hero->getGuiHero()->getPortraitLarge());

    m_ui->labelHeroName->setProperty("hoverName", heroName + ", " + heroClass);
    m_ui->labelHeroName->setProperty("popupDescr", m_hero->getGuiHero()->getBio());
    m_ui->labelHeroName->setProperty("popupAllowModal", true);

    m_ui->labelSpecialityIcon->setPixmap(m_hero->getGuiHero()->getSpecIcon());

    m_ui->labelSpecialityText->setText(
        formatTwoLine.arg(tr("Specialization"))
            .arg(m_hero->getGuiHero()->getSpecName()));

    m_ui->labelMoraleIcon->setValue(m_heroArmy->getSource()->squad.estimated.squadBonus.rngParams.morale);
    m_ui->labelLuckIcon->setValue(m_heroArmy->getSource()->squad.estimated.squadBonus.rngParams.luck);
    m_ui->labelMoraleIcon->setDetails(m_heroArmy->getSource()->squad.estimated.moraleDetails);
    m_ui->labelLuckIcon->setDetails(m_heroArmy->getSource()->squad.estimated.luckDetails);

    m_ui->primaryStatsWidget->setParams(m_hero->getSource()->estimated.primary);

    auto& expInfo  = m_modelsProvider->ui()->skillInfo[Core::HeroPrimaryParamType::Experience];
    auto& manaInfo = m_modelsProvider->ui()->skillInfo[Core::HeroPrimaryParamType::Mana];

    m_ui->labelExperienceIcon->setPixmap(expInfo.iconMedium->get());
    m_ui->labelManaIcon->setPixmap(manaInfo.iconMedium->get());

    m_ui->labelExperienceValue->setText(formatTwoLine
                                            .arg(expInfo.name)
                                            .arg(FormatUtils::formatLargeInt(m_hero->getSource()->experience)));
    auto remainExp = m_hero->getSource()->estimated.experienceNextLevel - m_hero->getSource()->experience;

    m_ui->labelExperienceValue->setProperty("popupDescr", expInfo.descr.arg(m_hero->getSource()->level).arg(FormatUtils::formatLargeInt(m_hero->getSource()->experience)).arg(FormatUtils::formatLargeInt(m_hero->getSource()->estimated.experienceNextLevel)).arg(FormatUtils::formatLargeInt(remainExp)));
    m_ui->labelExperienceValue->setProperty("popupAllowModal", true);

    m_ui->labelManaValue->setText(formatTwoLine
                                      .arg(manaInfo.name)
                                      .arg(QString("%1/%2").arg(m_hero->getSource()->mana).arg(m_hero->getSource()->estimated.maxMana)));

    m_ui->labelManaValue->setProperty("popupDescr", manaInfo.descr.arg(m_hero->getSource()->mana).arg(m_hero->getSource()->estimated.maxMana));
    m_ui->labelManaValue->setProperty("popupAllowModal", true);

    m_ui->skillsGridWidget->setParams(m_hero);

    m_ui->pushButtonUseCompactFormation->setChecked(m_heroArmy->getSquad()->getSource()->useCompactFormation);
    m_ui->pushButtonUseDistantFormation->setChecked(!m_heroArmy->getSquad()->getSource()->useCompactFormation);
}

void HeroMainDialog::deleteStack(AdventureStackConstPtr stack)
{
    IAdventureSquadControl::StackAction act;
    act.type = IAdventureSquadControl::StackActionType::Delete;
    act.from = stack;
    m_adventureSquadControl->heroStackAction(act);
}

void HeroMainDialog::showInfo(const GuiAdventureStack* stack, bool modal)
{
    if (!stack)
        return;
    auto advStack = stack->getSource();
    m_infoWidget.reset(new UnitInfoWidget(nullptr, advStack, m_modelsProvider, modal, this));

    connect(m_infoWidget.get(), &UnitInfoWidget::deleteStack, this, &HeroMainDialog::deleteStack);
    QPoint pos = this->mapToGlobal(QPoint(-10 + advStack->armyParams.indexInArmy * 66, 470 - m_infoWidget->sizeHint().height()));
    DialogUtils::moveWidgetWithinVisible(m_infoWidget.get(), pos);
    if (modal)
        m_infoWidget->exec();
    else
        m_infoWidget->show();
}

void HeroMainDialog::hideInfo()
{
    if (m_infoWidget && !m_infoWidget->isShowModal()) {
        m_infoWidget->hide();
        m_infoWidget.release()->deleteLater();
    }
}

void HeroMainDialog::openSpellBook()
{
    auto spells = m_hero->getSource()->estimated.availableSpells;

    SpellBookDialog dlg(spells,
                        m_modelsProvider,
                        m_hero->getSource()->mana,
                        false,
                        false,
                        this);
    dlg.exec();
}

void HeroMainDialog::updateGraphics()
{
    auto& heroDialog = m_modelsProvider->ui()->heroDialog;

    m_ui->pushButtonUseDistantFormation->setIcon(heroDialog.distantIcons->get());
    m_ui->pushButtonUseCompactFormation->setIcon(heroDialog.compactIcons->get());
    m_ui->pushButtonTacticsDisable->setIcon(heroDialog.tacticsIcons->get());
    m_ui->pushButtonSplit->setIcon(heroDialog.stackSplit->get());

    m_ui->pushButtonDeleteHero->setIcon(heroDialog.deleteHero->get());
    m_ui->pushButtonBag->setIcon(heroDialog.bag->get());
    m_ui->pushButtonListInfo->setIcon(heroDialog.listInfo->get());

    m_ui->labelFlagIcon->setPixmap(heroDialog.labelFlag->get());
}

}
