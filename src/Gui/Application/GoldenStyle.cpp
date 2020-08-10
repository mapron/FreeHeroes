/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "GoldenStyle.hpp"

#include <QComboBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStyleFactory>

#include <QDebug>

namespace FreeHeroes::Gui {

GoldenStyle::GoldenStyle() :
    QProxyStyle(QStyleFactory::create("Fusion"))
{
    setObjectName("FreeHeroesGolden");
}
QPalette GoldenStyle::standardPalette() const
{
    if (!m_standardPalette.isBrushSet(QPalette::Disabled, QPalette::Mid)) {

        QColor textGold(250, 228, 150);
        QColor brightYellow(224, 207, 140);
        QColor slightlyOpaqueBlack(0, 0, 0, 40);
        QColor slightlyOpaqueWhite(255, 255, 255, 20);

        QImage backgroundTextureImage(":/Application/back_brown.png");
        QImage backgroundTextureImageDarker = backgroundTextureImage.convertToFormat(QImage::Format_RGB32);
        QImage backgroundTextureImageLighter = backgroundTextureImage.convertToFormat(QImage::Format_RGB32);

        {
            QPainter painter;
            painter.begin(&backgroundTextureImageDarker);
            painter.setPen(Qt::NoPen);
            painter.fillRect(backgroundTextureImageDarker.rect(), slightlyOpaqueBlack);
            painter.end();
        }
        {
            QPainter painter;
            painter.begin(&backgroundTextureImageLighter);
            painter.setPen(Qt::NoPen);
            painter.fillRect(backgroundTextureImageLighter.rect(), slightlyOpaqueWhite);
            painter.end();
        }
        const QColor backgroundTextureImageAverageDark(69, 37, 9);
        const QColor backgroundTextureImageAverageBright(87, 55, 24);
        const QColor backgroundTextureImageAverageAvg(78, 46, 17);

        QBrush windowText = textGold.lighter();
        QBrush button(backgroundTextureImageAverageDark, QPixmap::fromImage(backgroundTextureImageDarker));
        QBrush light = backgroundTextureImageAverageBright;
        QBrush dark = backgroundTextureImageAverageDark.darker(200);

        light = QColor(0, 0, 0, 150);
        dark  = QColor(160, 160, 160, 128);

        QBrush mid (backgroundTextureImageAverageDark.darker(150), QPixmap::fromImage(backgroundTextureImageDarker));
        QBrush text = textGold;
        QBrush bright_text = QColor(Qt::white);
        QBrush base = QColor(0, 0, 0, 90);
        QBrush window(backgroundTextureImageAverageAvg, QPixmap::fromImage(backgroundTextureImage));

        window.setColor(backgroundTextureImageAverageBright.lighter(150));


        QPalette palette(windowText, button, light,
                 dark, mid, text,
                 bright_text, base, window);

       // palette
        palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 120));

        palette.setColor(QPalette::Highlight,brightYellow);
        palette.setColor(QPalette::HighlightedText, Qt::black);

        m_standardPalette = palette;
    }

    return m_standardPalette;
}

void GoldenStyle::polish(QWidget *widget)
{
    if (qobject_cast<QPushButton *>(widget)
            || qobject_cast<QComboBox *>(widget))
        widget->setAttribute(Qt::WA_Hover, true);
}

void GoldenStyle::unpolish(QWidget *widget)
{
    if (qobject_cast<QPushButton *>(widget)
            || qobject_cast<QComboBox *>(widget))
        widget->setAttribute(Qt::WA_Hover, false);
}

int GoldenStyle::pixelMetric(PixelMetric metric,
                             const QStyleOption *option,
                             const QWidget *widget) const
{
    return QProxyStyle::pixelMetric(metric, option, widget);
}

int GoldenStyle::styleHint(StyleHint hint, const QStyleOption *option,
                           const QWidget *widget,
                           QStyleHintReturn *returnData) const
{
    switch (hint) {
    case SH_DitherDisabledText:
        return int(false);
    case SH_EtchDisabledText:
        return int(true);
    case SH_ComboBox_Popup:
        return int(false);
    default:
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
}

void drawRectBox(QRect rect, QPainter *p, QColor c1, QColor c2, QColor c3, QColor c4, bool fill, QBrush brush) {
    int x = rect.x();
    int y = rect.y();
    int w = rect.width();
    int h = rect.height();

    QPen oldPen = p->pen();
    QPoint a[3] = { QPoint(x, y+h-2), QPoint(x, y), QPoint(x+w-2, y) };
    p->setPen(QPen(c1, 1.));
    p->drawPolyline(a, 3);

    QPoint b[3] = { QPoint(x, y+h-1), QPoint(x+w-1, y+h-1), QPoint(x+w-1, y) };
    p->setPen(QPen(c2, 1.));
    p->drawPolyline(b, 3);

    if (w > 4 && h > 4) {
        QPoint c[3] = { QPoint(x+1, y+h-3), QPoint(x+1, y+1), QPoint(x+w-3, y+1) };
        p->setPen(QPen(c3, 1.));
        p->drawPolyline(c, 3);

        QPoint d[3] = { QPoint(x+1, y+h-2), QPoint(x+w-2, y+h-2), QPoint(x+w-2, y+1) };
        p->setPen(QPen(c4, 1.));
        p->drawPolyline(d, 3);
        if (fill)
            p->fillRect(QRect(x+2, y+2, w-4, h-4), brush);
    }
}



void GoldenStyle::drawPrimitive(PrimitiveElement element,
                                const QStyleOption *option,
                                QPainter *painter,
                                const QWidget *widget) const
{
    switch (element) {
    case PE_PanelButtonCommand:
    {
        int delta = (option->state & State_MouseOver) ? 64 : 0;

        QColor semiTransparentWhite(255, 255, 255, 127 + delta);
        QColor semiTransparentBlack(0, 0, 0, 127 - delta);

        int x, y, width, height;
        option->rect.getRect(&x, &y, &width, &height);

        auto rect = option->rect;

        QBrush brush;
        bool darker = (option->state & (State_Sunken | State_On));

        const QStyleOptionButton *buttonOption =
                qstyleoption_cast<const QStyleOptionButton *>(option);
        if (buttonOption
                && (buttonOption->features & QStyleOptionButton::Flat)) {
            brush = option->palette.window();
        } else {
            if (option->state & (State_Sunken | State_On)) {
                brush = option->palette.mid();
            } else {
                brush = option->palette.button();
            }
        }

        {
            auto *p = painter;
            auto & pal = option->palette;
            auto c1 = pal.shadow().color();
            auto c2 = pal.light().color();
            auto c3 = pal.dark().color();
            auto c4 = QColor(0, 0, 0, 120);//pal.button().color();
            if (darker) {
                qSwap(c1, c2);
                qSwap(c3, c4);
            }

            drawRectBox(rect, p, c1, c2, c3, c4, true, brush);
        }
    }
        break;
    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

}

void GoldenStyle::drawControl(ControlElement element,
                              const QStyleOption *option,
                              QPainter *p,
                              const QWidget *widget) const
{

    switch (element) {
    case CE_ShapedFrame:
        if (const QStyleOptionFrame *f = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            if (f->frameShape ==  QFrame::Box || f->frameShape ==  QFrame::Panel) {
                if (widget->property("dbg").toInt() == 1) {
                    p->fillRect(f->rect, QColor(255,0,0));
                    return;
                }

                QList<QColor> colors {
                   /*0*/ QColor(0, 0, 0) ,
                   /*1*/ QColor(82, 60, 24),
                   /*2*/ QColor(173, 140, 66),
                   /*3*/ QColor(255, 239, 150),
                   /*4*/ QColor(250, 240, 190) ,
                   /*5*/ QColor(250, 240, 190, 150),
                   /*6*/ QColor(50, 50, 50, 100),
                   /*7*/ QColor(255, 255, 255, 160),
                };
                static const QMap<QString, QStringList> configs {
                    //                         bottom       right        left         top
                    {"main"      , QStringList {"03200321",   "03200321" ,    "12300123", "12300321"} },
                    {"common"    , QStringList {     "321",        "321" ,         "123",      "321"} },
                    {"common2"   , QStringList {     "321",        "321" ,         "321",      "321"} },
                    {"commonDark", QStringList {      "05",         "05" ,          "50",       "50"} },
                    {"softDark"  , QStringList {      "76",         "56" ,          "65",       "67"} },
                    {"wide"      , QStringList {      "321999920", "321999920" ,   "123999920","321999920"} },
                };

                QString borderStyle    = widget->property("borderStyle").toString();
                QStringList cfgs = configs.value(borderStyle);
                QString bottomBorderCfg  = cfgs.value(0);
                QString rightBorderCfg   = cfgs.value(1);
                QString leftBorderCfg    = cfgs.value(2);
                QString topBorderCfg     = cfgs.value(3);

                const int leftBottomFix = widget->property("leftBottomFix").toInt();
                const int leftTopFix = widget->property("leftTopFix").toInt();

                const bool fill = widget->property("fill").toBool();

                const auto bottomVar = widget->property("bottomBorder");
                const auto rightVar  = widget->property("rightBorder" );
                const auto leftVar   = widget->property("leftBorder"  );
                const auto topVar    = widget->property("topBorder"   );
                if (bottomVar.isValid())
                    bottomBorderCfg  = bottomVar.toString();
                if (rightVar.isValid())
                    rightBorderCfg   = rightVar.toString();
                if (leftVar.isValid())
                    leftBorderCfg    = leftVar.toString();
                if (topVar.isValid())
                    topBorderCfg     = topVar.toString();

                [[maybe_unused]] const int dbg = widget->property("dbg").toInt();
                QColor specialColor = Qt::transparent;
                if (widget->property("specialColor").isValid()) {
                    specialColor = widget->property("specialColor").value<QColor>();
                }
                //if (dbg == 1) {
                    //p->fillRect(option->rect, Qt::red);
                    //return;
                //}


                const int bottomLineHeight = bottomBorderCfg.size();
                const int rightLineWidth   = rightBorderCfg.size();
                const int topLineHeight    = topBorderCfg.size();
                const int leftLineWidth    = leftBorderCfg.size();

                auto getColor = [&colors, &specialColor, f](const QString & cfg, int index) -> QColor {
                    if (index >= cfg.size())
                        return Qt::transparent;
                    auto clr = colors.value(cfg.mid(index, 1).toInt(), specialColor);
                    if (f->frameShape ==  QFrame::Panel && clr.alpha() > 100) {
                        clr.setAlpha(clr.alpha() - 100);
                    }
                    return clr;
                };

                const int maxThin = std::max({bottomLineHeight, rightLineWidth, topLineHeight, leftLineWidth});
                int bottomLineAdjust = 1;
                int rightLineAdjust  = 1;
                int topLineAdjust    = 1;
                int leftLineAdjust   = 1;
                QRect r =  option->rect;
                for (int i = 0; i < maxThin; i++) {
                    QColor bottom = getColor(bottomBorderCfg, i);
                    QColor right  = getColor(rightBorderCfg , i);
                    QColor top    = getColor(topBorderCfg   , i);
                    QColor left   = getColor(leftBorderCfg  , i);

                    {

                        const int x = r.x();
                        const int y = r.y();
                        const int w = r.width();
                        const int h = r.height();
                        const int b = y + h - 1;
                        const int r = x + w - 1;
                        const int lbFix = leftBottomFix > 0 && i < leftBottomFix ? (leftBottomFix - i) : 0;
                        const int ltFix = leftTopFix    > 0 && i < leftTopFix    ? (leftTopFix    - i) : 0;

                        p->setPen(QPen(left, 1.));
                        p->drawLine(QPoint(x, y - ltFix), QPoint(x, b + lbFix));

                        p->setPen(QPen(right, 1.));
                        p->drawLine(QPoint(r, y), QPoint(r, b));

                        p->setPen(QPen(top, 1.));
                        p->drawLine(QPoint(x + ltFix, y), QPoint(r, y));

                        p->setPen(QPen(bottom, 1.));
                        p->drawLine(QPoint(x + lbFix, b), QPoint(r, b));
                    }

                    r.adjust(leftLineAdjust, topLineAdjust, -(rightLineAdjust ), -(bottomLineAdjust));
                    if (i >= topLineHeight - 1)
                        topLineAdjust = 0;
                    if (i >= leftLineWidth - 1)
                        leftLineAdjust = 0;
                    if (i >= bottomLineHeight - 1)
                        bottomLineAdjust = 0;
                    if (i >= rightLineWidth - 1)
                        rightLineAdjust = 0;
                }
                if (fill) {
                    p->fillRect(r, QColor(0,0,0, 80));
                }

                if (borderStyle == "wide") {
                    QImage img(":/Application/corner.png");
                    QImage topRight    = img.mirrored(true, false);
                    QImage bottomRight = img.mirrored(true, true);
                    QImage bottomLeft  = img.mirrored(false, true);
                    p->drawImage(f->rect.x() + 1, f->rect.y() + 1, img);
                    p->drawImage(f->rect.right() - topRight.width() - 1, f->rect.y() + 1, topRight);
                    p->drawImage(f->rect.right() - bottomRight.width() - 1, f->rect.bottom() - bottomRight.height() - 1, bottomRight);
                    p->drawImage(f->rect.x() + 1, f->rect.bottom() - bottomLeft.height() - 1, bottomLeft);
                }

                return;
            }
            if (f->frameShape ==  QFrame::HLine || f->frameShape ==  QFrame::VLine) {
                QRect rec = option->rect;
                QPoint p1, p2, p3, p4;
                if (f->frameShape == QFrame::HLine) {
                    rec.adjust(+10, 0, -10, 0);
                    p1 = QPoint(rec.x(), rec.y() + rec.height() / 2);
                    p2 = QPoint(rec.x() + rec.width(), p1.y());
                    p3 = p1 + QPoint(0, 1);
                    p4 = p2 + QPoint(0, 1);
                } else {
                    rec.adjust(0, +10, 0, -10);
                    p1 = QPoint(rec.x() + rec.width() / 2, rec.y());
                    p2 = QPoint(p1.x(), p1.y() + rec.height());
                    p3 = p1 + QPoint(1, 0);
                    p4 = p2 + QPoint(1, 0);
                }


                p->setPen(QPen(QColor(250, 240, 190, 100), 1));
                p->drawLine(p1, p2);
                p->setPen(QPen(QColor(0, 0, 0, 80), 1));
                p->drawLine(p3, p4);

                return;
            }
        } break;
    default:
        break;
    }

    QProxyStyle::drawControl(element, option, p, widget);
}

void GoldenStyle::setTexture(QPalette &palette, QPalette::ColorRole role,
                             const QImage &image)
{
    for (int i = 0; i < QPalette::NColorGroups; ++i) {
        QBrush brush(image);
        brush.setColor(palette.brush(QPalette::ColorGroup(i), role).color());
        palette.setBrush(QPalette::ColorGroup(i), role, brush);
    }
}

QPainterPath GoldenStyle::roundRectPath(const QRect &rect)
{
    int radius = qMin(rect.width(), rect.height()) / 2;
    int diam = 2 * radius;

    int x1, y1, x2, y2;
    rect.getCoords(&x1, &y1, &x2, &y2);

    QPainterPath path;
    path.moveTo(x2, y1 + radius);
    path.arcTo(QRect(x2 - diam, y1, diam, diam), 0.0, +90.0);
    path.lineTo(x1 + radius, y1);
    path.arcTo(QRect(x1, y1, diam, diam), 90.0, +90.0);
    path.lineTo(x1, y2 - radius);
    path.arcTo(QRect(x1, y2 - diam, diam, diam), 180.0, +90.0);
    path.lineTo(x1 + radius, y2);
    path.arcTo(QRect(x2 - diam, y2 - diam, diam, diam), 270.0, +90.0);
    path.closeSubpath();
    return path;
}
}
