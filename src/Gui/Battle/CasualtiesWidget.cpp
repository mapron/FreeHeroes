/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "CasualtiesWidget.hpp"

#include "DependencyInjector.hpp"
#include "CustomFrames.hpp"

#include <QBoxLayout>
#include <QLabel>
#include <QPainter>

namespace FreeHeroes::Gui {

struct CasualtiesWidget::Impl {
};

CasualtiesWidget::CasualtiesWidget(QWidget* parent)
    : QWidget(parent)
    , m_impl(std::make_unique<Impl>())

{
    this->setFixedHeight(60);
    this->setProperty("borderStyle", "commonDark");
    this->setProperty("fill", true);
}

void CasualtiesWidget::setResultInfo(const CasualtiesWidget::Info& info)
{
    m_info = info;
    update();
}

void CasualtiesWidget::paintEvent(QPaintEvent*)
{
    QPainter          p(this);
    QStyleOptionFrame opt;
    opt.initFrom(this);
    opt.frameShape = QFrame::Shape::Panel;
    opt.rect.adjust(0, 0, 0, -20);
    this->style()->drawControl(QStyle::CE_ShapedFrame, &opt, &p, this);

    int       unitCount        = m_info.units.size();
    const int unitPixmapWidth  = 32;
    const int unitPixmapHeight = 32;
    const int topOffset        = 3;
    const int verticalSpacing  = 4;
    const int horSpacing       = 10;

    QRect r = this->rect().adjusted(0, 0, -1, -1);

    if (m_info.units.isEmpty()) {
        QString str             = tr("None");
        auto    fm              = QFontMetrics(this->font());
        int     numberStrWidth  = fm.horizontalAdvance(str);
        int     numberStrHeight = fm.height();
        p.setPen(Qt::white);
        p.drawText(r.width() / 2 - numberStrWidth / 2, topOffset + unitPixmapWidth / 2 + numberStrHeight / 2, str);
        return;
    }

    QFont f = this->font();
    f.setPixelSize(12);
    f.setStyleStrategy(QFont::StyleStrategy(f.styleStrategy() | QFont::NoSubpixelAntialias));
    p.setFont(f);

    QStringList numberLabels;
    QList<int>  numberWidths;
    int         maxLabelWidth   = 0;
    const int   numberStrHeight = QFontMetrics(f).height();
    for (auto& unit : m_info.units) {
        QString numberStr      = QString::number(unit.count);
        int     numberStrWidth = QFontMetrics(f).horizontalAdvance(numberStr);
        numberLabels << numberStr;
        numberWidths << numberStrWidth;
        maxLabelWidth = std::max(maxLabelWidth, numberStrWidth);
    }
    const int unitSpace      = std::max(0, maxLabelWidth - unitPixmapWidth) + horSpacing;
    const int cellWidth      = unitPixmapWidth + unitSpace;
    const int infoWidth      = unitPixmapWidth * unitCount + unitSpace * (unitCount - 1);
    const int offsetInWidget = r.width() / 2 - infoWidth / 2;
    for (int index = 0; index < m_info.units.size(); index++) {
        QPixmap   pix            = m_info.units[index].portrait;
        const int unitOffsetLeft = offsetInWidget + cellWidth * index;
        p.drawPixmap(unitOffsetLeft, topOffset, pix);
        const int labelOffsetCenter = unitOffsetLeft + pix.width() / 2;
        p.setPen(m_info.units[index].isDead ? Qt::red : Qt::white);
        p.drawText(labelOffsetCenter - numberWidths[index] / 2, topOffset + unitPixmapHeight + verticalSpacing + numberStrHeight, numberLabels[index]);
    }
}

CasualtiesWidget::~CasualtiesWidget() = default;

}
