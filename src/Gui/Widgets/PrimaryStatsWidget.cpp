/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "PrimaryStatsWidget.hpp"

#include "CustomFrames.hpp"
#include "HoverHelper.hpp"
#include "LibraryModels.hpp"
#include "UiCommonModel.hpp"

#include "Stat.hpp"

#include <QLabel>
#include <QBoxLayout>

namespace FreeHeroes::Gui {
using namespace Core;

PrimaryStatsWidget::PrimaryStatsWidget(QWidget* parent)
    : QWidget(parent)
    , m_hoverHelper(std::make_unique<HoverHelper>(this))
{
    m_params = { Core::HeroPrimaryParamType::Attack, Core::HeroPrimaryParamType::Defense, Core::HeroPrimaryParamType::SpellPower, Core::HeroPrimaryParamType::Intelligence };

    for (int i = 0; i < 4; ++i) {
        m_iconsLabels[i] = new DarkFrameLabelIcon(this);
        m_valueTxt[i]    = new QLabel(this);
        m_titleTxt[i]    = new QLabel(this);

        m_hoverHelper->addAlias(m_titleTxt[i], m_iconsLabels[i]);
        m_titleTxt[i]->setProperty("hoverName", m_titleTxt[i]->text() + ". " + tr("Show information"));
        m_titleTxt[i]->setProperty("popupOffsetAnchorHor", "center");
        m_titleTxt[i]->setProperty("popupOffset", QPoint(40 + i * 70, 90));

        m_titleTxt[i]->setProperty("popupAllowModal", true);
    }

    for (auto* label : m_titleTxt) {
        label->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);
        label->setMargin(3);
        auto p = label->palette();
        p.setBrush(QPalette::WindowText, p.brush(QPalette::Text));
        label->setPalette(p);
    }
    for (auto* label : m_valueTxt) {
        label->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);
        label->setMargin(3);
        label->setContentsMargins(0, 0, 2, 0);
    }

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setMargin(0);

    DarkFrame*   topLayoutFrame = new DarkFrame(this);
    QHBoxLayout* topLayout      = new QHBoxLayout(topLayoutFrame);
    topLayout->setMargin(0);
    mainLayout->addWidget(topLayoutFrame);
    for (auto* label : m_titleTxt)
        topLayout->addWidget(label);
    for (auto* label : m_titleTxt)
        topLayout->setStretchFactor(label, 1);

    QHBoxLayout* midLayout = new QHBoxLayout();
    mainLayout->addLayout(midLayout);
    midLayout->addSpacing(12);
    midLayout->setSpacing(25);
    for (auto* lbl : m_iconsLabels)
        midLayout->addWidget(lbl);
    midLayout->addSpacing(12);

    DarkFrame*   botLayoutFrame = new DarkFrame(this);
    QHBoxLayout* botLayout      = new QHBoxLayout(botLayoutFrame);
    botLayout->setMargin(0);
    botLayout->setSpacing(0);
    mainLayout->addWidget(botLayoutFrame);
    for (auto* label : m_valueTxt)
        botLayout->addWidget(label);
    for (auto* label : m_valueTxt)
        botLayout->setStretchFactor(label, 1);
}

void PrimaryStatsWidget::setParams(const HeroPrimaryParams& primary, const LibraryModelsProvider* modelsProvider)
{
    QStringList values{
        QString("%1").arg(primary.ad.attack),
        QString("%1").arg(primary.ad.defense),
        QString("%1").arg(primary.magic.spellPower),
        QString("%1").arg(primary.magic.intelligence),
    };
    auto& statsMap = modelsProvider->ui()->skillInfo;
    for (int i = 0; i < 4; ++i) {
        auto& skillInfo = statsMap[m_params[i]];
        m_valueTxt[i]->setText(values[i]);
        m_titleTxt[i]->setText(skillInfo.name);
        m_titleTxt[i]->setProperty("popupBottom", values[i] + " " + m_titleTxt[i]->text());
        m_titleTxt[i]->setProperty("popupPixmap", skillInfo.iconLarge->get());
        m_titleTxt[i]->setProperty("popupDescr", skillInfo.descr);
        m_iconsLabels[i]->setPixmap(skillInfo.iconMedium->get());
    }
}

PrimaryStatsWidget::~PrimaryStatsWidget() = default;

void PrimaryStatsWidget::setHoverLabel(QLabel* hoverLabel)
{
    m_hoverHelper->setHoverLabel(hoverLabel);
}

}
