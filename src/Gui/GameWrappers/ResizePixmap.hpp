/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QPixmap>
#include <QPainter>

namespace FreeHeroes::Gui {

inline QPixmap nearestNeighbour(QPixmap source, double scale)
{
    QImage src = source.toImage();
    QImage img(src.size() * scale, QImage::Format_RGBA8888);
    for (int h=0; h < img.height(); ++h) {
        for (int w=0; w < img.width(); ++w) {
            int srcW = w / scale;
            int srcH = h / scale;
            img.setPixel(w, h, src.pixel(srcW, srcH));
        }
    }
    return QPixmap::fromImage(img);
}

inline QPixmap resizePixmap(QPixmap source, QSize maxSize, bool useNN = false)
{
    if (source.isNull())
        return source;

    const int shrinkWidth  = maxSize.width()  - source.width();
    const int shrinkHeight = maxSize.height() - source.height();
    if (maxSize.isValid() && (shrinkWidth < 0 || shrinkHeight < 0)) {

        if (useNN) {
            const double ratioWidth   = (double)source.width()  / maxSize.width();
            const double ratioHeight  = (double)source.height() / maxSize.height();
            const double ratio = std::max(ratioWidth, ratioHeight);
            source = nearestNeighbour(source, 1 / ratio);
        } else {
            auto sourceImage = source.toImage();
            sourceImage = sourceImage.scaled(maxSize, Qt::KeepAspectRatio, Qt::FastTransformation);
            source = QPixmap::fromImage(sourceImage);
        }
    }

    return source;
}


}
