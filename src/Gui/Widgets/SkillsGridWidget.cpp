/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "SkillsGridWidget.hpp"

// Gui
#include "CustomFrames.hpp"
#include "AdventureWrappers.hpp"
#include "HoverHelper.hpp"
#include "LibraryWrappersMetatype.hpp"
#include "LibraryModels.hpp"

// Core
#include "AdventureHero.hpp"
#include "LibrarySecondarySkill.hpp"

#include <QLabel>
#include <QVariant>
#include <QGridLayout>

namespace FreeHeroes::Gui {
using namespace Core;

namespace {

const int rows = 4;
const int cols = 2;

const QString formatTwoLine = QString("<p style=\"line-height:60%\">%1</p><p>%2</p>");

}

SkillsGridWidget::SkillsGridWidget(const LibraryModelsProvider* modelProvider, QWidget* parent)
    : QWidget(parent)
    , m_hoverHelper(std::make_unique<HoverHelper>(modelProvider, this))
{
    QGridLayout* layout = new QGridLayout(this);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            QWidget*     wrap       = new QWidget(this);
            QHBoxLayout* wrapLayout = new QHBoxLayout(wrap);
            wrapLayout->setMargin(0);
            wrapLayout->setSpacing(0);

            auto icon = new DarkFrameLabelIcon(this);
            icon->setFixedSize(QSize{ 44, 44 } + QSize{ 4, 4 });
            wrapLayout->addWidget(icon);

            auto lbl = new DarkFrameLabel(this);
            lbl->setMinimumWidth(95);
            lbl->setFixedHeight(48);
            lbl->setMargin(3);
            wrapLayout->addWidget(lbl);

            m_icons << icon;
            m_txts << lbl;

            QPoint offset(290, 0);
            lbl->setProperty("popupOffset", offset);
            lbl->setProperty("popupOffsetAnchorVert", "center");

            layout->addWidget(wrap, r, c * 2);
            m_hoverHelper->addAlias(lbl, icon);
        }
    }
    layout->setColumnStretch(1, 1);
    layout->setSpacing(0);
    layout->setMargin(0);
}

void SkillsGridWidget::setParams(const GuiAdventureHero* hero)
{
    int index      = 0;
    int skillCount = (int) hero->getSource()->secondarySkills.size();
    for (index = 0; index < skillCount; ++index) {
        auto          skillsModel = hero->getSkillsEditModel();
        const auto    level       = skillsModel->index(index, 1).data(Qt::EditRole).toInt();
        const auto    levelStr    = skillsModel->index(index, 1).data(Qt::DisplayRole).toString();
        const auto    guiSkill    = skillsModel->index(index, 0).data(SkillsModel::GuiObject).value<GuiSkillConstPtr>();
        const QString name        = guiSkill->getName();
        const QString descr       = guiSkill->getDescription(level);
        QPixmap       imgMed      = guiSkill->getIconMedium(level);
        QPixmap       imgLarge    = guiSkill->getIconLarge(level);

        const QString title = formatTwoLine.arg(levelStr).arg(name);

        m_txts[index]->setText(title);
        m_icons[index]->setPixmap(imgMed);

        QWidget* w = m_txts[index];
        w->setProperty("hoverName", tr("Show information about skill: ") + name);
        w->setProperty("popupDescr", descr);
        w->setProperty("popupPixmap", QVariant::fromValue(imgLarge));
        w->setProperty("popupBottom", title);
        w->setProperty("popupAllowModal", true);
    }
    for (; index < rows * cols; ++index) {
        m_txts[index]->setText("");
        m_icons[index]->setPixmap({});
        QWidget* w = m_txts[index];
        w->setProperty("hoverName", {});
        w->setProperty("popupDescr", {});
        w->setProperty("popupPixmap", {});
        w->setProperty("popupBottom", {});
    }
}

SkillsGridWidget::~SkillsGridWidget() = default;

void SkillsGridWidget::setHoverLabel(QLabel* hoverLabel)
{
    m_hoverHelper->setHoverLabel(hoverLabel);
}

}
