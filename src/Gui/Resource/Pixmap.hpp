/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <vector>
#include <memory>
#include <algorithm>

#include "MernelPlatform/FsUtils.hpp"
#include "MernelPlatform/ByteBuffer.hpp"

#include "GuiResourceExport.hpp"

class QPixmap;
class QSize;
class QPoint;
class QColor;

namespace FreeHeroes {

struct GUIRESOURCE_EXPORT PixmapPoint {
    int m_x = 0;
    int m_y = 0;

    constexpr inline PixmapPoint& operator+=(const PixmapPoint& right_)
    {
        m_x += right_.m_x;
        m_y += right_.m_y;
        return *this;
    }
    bool isNull() const
    {
        return m_x == 0 && m_y == 0;
    }

    QPoint toQPoint() const noexcept;

    auto operator<=>(const PixmapPoint&) const = default;
};
constexpr inline PixmapPoint operator+(const PixmapPoint& left_, const PixmapPoint& right_)
{
    return { left_.m_x + right_.m_x, left_.m_y + right_.m_y };
}
constexpr inline PixmapPoint operator-(const PixmapPoint& left_, const PixmapPoint& right_)
{
    return { left_.m_x - right_.m_x, left_.m_y - right_.m_y };
}

struct GUIRESOURCE_EXPORT PixmapSize {
    int m_width  = 0;
    int m_height = 0;

    bool isNull() const
    {
        return m_width == 0 && m_height == 0;
    }
    auto operator<=>(const PixmapSize&) const = default;

    QSize toQSize() const noexcept;
};

struct GUIRESOURCE_EXPORT PixmapColor {
    uint8_t m_r = 0;
    uint8_t m_g = 0;
    uint8_t m_b = 0;
    uint8_t m_a = 0;

    auto operator<=>(const PixmapColor&) const = default;

    PixmapColor() = default;
    PixmapColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : m_r(r)
        , m_g(g)
        , m_b(b)
        , m_a(a)
    {}

    explicit PixmapColor(const std::string& str) noexcept { fromString(str); }

    std::string toString() const noexcept;
    void        fromString(const std::string& str) noexcept;

    bool isValid() const { return *this != PixmapColor{}; }

    QColor toQColor() const noexcept;
};

struct GUIRESOURCE_EXPORT Pixmap {
    Pixmap() = default;
    explicit Pixmap(PixmapSize size)
        : m_size(size)
    {
        updateSize();
    }
    Pixmap(int width, int height)
        : Pixmap(PixmapSize{ width, height })
    {
    }

    bool isNull() const { return m_pixels.empty(); }

    void fill(PixmapColor color)
    {
        for (auto& pix : m_pixels)
            pix.m_color = color;
    }

    struct Pixel {
        PixmapColor m_color;
    };

    PixmapSize m_size;

    std::vector<Pixel> m_pixels;

    Pixel& get(int x, int y)
    {
        const int yOffset = m_size.m_width * y;
        return m_pixels[yOffset + x];
    }
    const Pixel& get(int x, int y) const
    {
        const int yOffset = m_size.m_width * y;
        return m_pixels[yOffset + x];
    }

    [[nodiscard]] int correctX(int x) const
    {
        return std::clamp(x, 0, m_size.m_width - 1);
    }
    [[nodiscard]] int correctY(int y) const
    {
        return std::clamp(y, 0, m_size.m_height - 1);
    }
    bool inBounds(int x, int y) const
    {
        return x >= 0 && x < m_size.m_width && y >= 0 && y < m_size.m_height;
    }

    size_t totalPixelSize() const
    {
        return m_size.m_width * m_size.m_height;
    }
    size_t totalByteSize() const
    {
        return totalPixelSize() * 4;
    }

    int width() const { return m_size.m_width; }
    int height() const { return m_size.m_height; }

    void updateSize()
    {
        m_pixels.resize(totalPixelSize());
    }

    void loadPng(const Mernel::std_path& path);
    void savePng(const Mernel::std_path& path) const;

    void loadPngFromBuffer(const Mernel::ByteArrayHolder& holder);
    void savePngToBuffer(Mernel::ByteArrayHolder& holder) const;

    void loadBmp(const Mernel::std_path& path);

    Pixmap subframe(const PixmapPoint& offset, const PixmapSize& size) const;
    Pixmap padToSize(const PixmapSize& size, const PixmapPoint& leftTop) const;
    void   flipVertical();

    QPixmap toQtPixmap() const;
};

}
