/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "Pixmap.hpp"

#include "GuiResourceExport.hpp"

namespace FreeHeroes {

class GUIRESOURCE_EXPORT Painter {
public:
    Painter(Pixmap* canvas)
        : m_canvas(*canvas)
    {
    }

    void drawPixmap(const PixmapPoint& offset, const Pixmap& pixmap, bool flipHor = false, bool flipVert = false);
    void drawRect(const PixmapPoint& topLeft, const PixmapSize& size, const PixmapColor& color);

    void translate(int x, int y)
    {
        m_offset.m_x += x;
        m_offset.m_y += y;
    }
    void translate(const PixmapPoint& offset)
    {
        m_offset.m_x += offset.m_x;
        m_offset.m_y += offset.m_y;
    }

    PixmapPoint getTransform() const { return m_offset; }
    void        setTransform(PixmapPoint offset) { m_offset = offset; }

private:
    PixmapPoint m_offset;
    Pixmap&     m_canvas;
};

}
