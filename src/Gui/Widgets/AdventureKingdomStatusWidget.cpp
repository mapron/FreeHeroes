/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AdventureKingdomStatusWidget.hpp"

#include "LibraryResource.hpp"

#include "CustomFrames.hpp"
#include "ResizePixmap.hpp"

#include <QBoxLayout>
#include <QPainter>

namespace FreeHeroes::Gui {

namespace {
const int resCount       = 7;
const int resLabelWidth  = 80;
const int resLabelSpacer = 6;
const int goldLabelWidth = 120;
}

struct AdventureKingdomStatusWidget::Impl {
    struct ResWidget {
        DarkFrameLabel* text = nullptr;
        QLabel*         img  = nullptr;

        void set(int current, int income)
        {
            QString txt = QString("<b>%1</b>").arg(current);
            if (income > 0)
                txt += QString(" (+%1)").arg(income);
            text->setText(txt);
        }
    };

    ResWidget wood;
    ResWidget mercury;
    ResWidget ore;

    ResWidget sulfur;
    ResWidget crystal;
    ResWidget gems;

    ResWidget gold;

    QList<ResWidget*> m_resLabels;
    QStringList       m_resIds;

    QLabel* dateLabel = nullptr;
};

AdventureKingdomStatusWidget::AdventureKingdomStatusWidget(QWidget* parent)
    : QWidget(parent)
    , m_impl(std::make_unique<Impl>())
{
    m_impl->m_resLabels = {
        &m_impl->wood,
        &m_impl->mercury,
        &m_impl->ore,

        &m_impl->sulfur,
        &m_impl->crystal,
        &m_impl->gems,

        &m_impl->gold,
    };
    m_impl->m_resIds = QStringList{
        "wood",
        "mercury",
        "ore",

        "sulfur",
        "crystal",
        "gems",

        "gold",
    };

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < resCount; i++) {
        Impl::ResWidget& res = *m_impl->m_resLabels[i];
        res.text             = new DarkFrameLabel(this);
        res.text->setAlignment(Qt::AlignLeft);
        res.text->setIndent(30);
        res.text->setFixedWidth(i == resCount - 1 ? goldLabelWidth : resLabelWidth);
        layout->addWidget(res.text);
        res.text->setProperty("fill", false);

        res.img = new QLabel(this);
        res.img->setFixedSize(24, 24);
        res.img->move(3 + i * (resLabelWidth + resLabelSpacer), -2);
    }
    layout->addStretch(1);
    m_impl->dateLabel = new QLabel(this);
    layout->addWidget(m_impl->dateLabel);
    layout->addSpacing(10);
}

void AdventureKingdomStatusWidget::setResourceIcons(const QMap<QString, QPixmap>& icons)
{
    for (int i = 0; i < resCount; i++) {
        const auto& id    = m_impl->m_resIds[i];
        auto*       label = m_impl->m_resLabels[i];
        label->img->setPixmap(icons.value(id));
    }
}

void AdventureKingdomStatusWidget::update(const Core::AdventureKingdom& kingdom)
{
    for (int i = 0; i < resCount; i++) {
        const std::string id     = m_impl->m_resIds[i].toStdString();
        auto*             widget = m_impl->m_resLabels[i];
        int               cur    = 0;
        int               inc    = 0;

        for (const auto& [res, value] : kingdom.currentResources.data)
            if (res->id == id)
                cur = value;
        for (const auto& [res, value] : kingdom.dayIncome.data)
            if (res->id == id)
                inc = value;

        widget->set(cur, inc);
    }

    m_impl->dateLabel->setText(tr("Month: %1, Week: %2, Day: %3")
                                   .arg(kingdom.currentDate.month)
                                   .arg(kingdom.currentDate.week)
                                   .arg(kingdom.currentDate.day));
}

void AdventureKingdomStatusWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.fillRect(rect().adjusted(0, 0, -1, -1), QBrush(QColor(230, 0, 0, 100))); // @todo: red color sharing!

    QWidget::paintEvent(event);
}

AdventureKingdomStatusWidget::~AdventureKingdomStatusWidget() = default;

}
