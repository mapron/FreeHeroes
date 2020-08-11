/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "RngParamLabel.hpp"

#include "DependencyInjector.hpp"
#include "LibraryModels.hpp"
#include "UiCommonModel.hpp"

#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStringList>

namespace FreeHeroes::Gui {

RngLabel::RngLabel(QWidget* parent) : DarkFrameLabelIcon(parent),
    m_ui(*loadDependency<LibraryModelsProvider>(parent).ui()){
    auto ui = loadDependency<IAppSettings>(parent).ui();
    m_displayText = ui.displayAbsMoraleLuck;
    m_clampText = ui.clampAbsMoraleLuck;
}

RngLabel::~RngLabel() = default;

void RngLabel::setValue(int bonusValue)
{
    m_value = bonusValue;
    const int clampedValue = std::clamp(bonusValue, -3, +3);
    //const int index = 3 + clampedValue;
    auto pix = (m_isLuck ? m_ui.luck : m_ui.morale).medium.value(clampedValue)->get();

    auto largePixmap = (m_isLuck ? m_ui.luck : m_ui.morale).large.value(clampedValue)->get();
    setProperty("popupPixmap", largePixmap);
    QString valueStr = QString::number(m_clampText ? clampedValue : bonusValue);
    if (bonusValue > 0)
        valueStr = "+" + valueStr;

    QImage img(pix.size() + QSize(20, 0), QImage::Format_RGBA8888);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.drawPixmap(0,0, pix);
    if (m_displayText) {
        auto f = p.font();
        f.setPointSize(11);
        QFontMetrics fm(f);
        const int textWidth = fm.horizontalAdvance(valueStr);
        const int textHeight = fm.height();
        p.setFont(f);

        const int textX = img.width() - textWidth - 6;
        const int textY = (img.height() + textHeight) / 2 - 5;

        QPainterPath myPath;
        myPath.addText(textX, textY, f, valueStr);

        QPainterPathStroker stroker;
        stroker.setWidth( 3 );
        const QPainterPath stroked = stroker.createStroke( myPath );

        p.setBrush(Qt::black);
        p.setPen(Qt::NoPen);
        p.drawPath(stroked);

        p.setBrush(Qt::black);
        p.setPen(Qt::white);
        p.drawText(QPoint{textX, textY}, valueStr);
    }

    pix = QPixmap::fromImage(img);
    this->setPixmap(pix);
}


LuckLabel::LuckLabel(QWidget* parent) : RngLabel(parent) { m_isLuck = true;}

void LuckLabel::setDetails(const Core::LuckDetails& details)
{
    QStringList parts;
    if (m_value > 0) {
        parts << tr("{Positive luck}");
        parts << "";
        parts << tr("Positive luck give you chance to have extra base damage");
    } else if (m_value < 0) {
        parts << tr("{Negative luck}");
        parts << "";
        parts << tr("Negative luck give you chance to have reduced overall damage");
    } else {
        parts << tr("{Neutral luck}");
        parts << "";
        parts << tr("Neutral luck give no special effects");
    }
    parts << "";
    parts << "";
    parts << tr("{Current luck modificators}");
    parts << m_ui.getLuckDescription(details);
    setProperty("popupDescr", parts.join("<br>"));
    setProperty("hoverName", tr("Show details about luck"));
    setProperty("popupAllowModal", true);
}

MoraleLabel::MoraleLabel(QWidget* parent) : RngLabel(parent) { m_isLuck = false;}

void MoraleLabel::setDetails(const Core::MoraleDetails& details)
{
    QStringList parts;
    if (m_value > 0) {
        parts << tr("{Positive morale}");
        parts << "";
        parts << tr("Positive morale give you chance to have extra move in a battle");
    } else if (m_value < 0) {
        parts << tr("{Negative morale}");
        parts << "";
        parts << tr("Negative morale give you chance to skip one move in a battle");
    } else {
        parts << tr("{Neutral morale}");
        parts << "";
        parts << tr("Neutral morale give no special effects");
    }
    parts << "";
    parts << "";
    parts << tr("{Current morale modificators}");
    parts << m_ui.getMoraleDescription(details);
    setProperty("popupDescr", parts.join("<br>"));
    setProperty("hoverName", tr("Show details about morale"));
    setProperty("popupAllowModal", true);
}


}
