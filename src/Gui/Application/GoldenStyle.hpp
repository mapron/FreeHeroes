/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QProxyStyle>

namespace FreeHeroes::Gui {

class GoldenStyle : public QProxyStyle {
    Q_OBJECT

public:
    GoldenStyle();

    QPalette standardPalette() const override;

    void polish(QWidget* widget) override;
    void unpolish(QWidget* widget) override;
    int  pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override;
    int  styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
    void drawControl(ControlElement control, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;

private:
    static void         setTexture(QPalette& palette, QPalette::ColorRole role, const QImage& image);
    static QPainterPath roundRectPath(const QRect& rect);
    mutable QPalette    m_standardPalette;
};

}
