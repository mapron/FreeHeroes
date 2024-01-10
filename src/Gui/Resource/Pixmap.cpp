/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "Pixmap.hpp"

#include "MernelPlatform/FileIOUtils.hpp"

#ifdef _MSC_VER
#pragma warning(disable : 4505)
#endif

#define STBI_NO_JPEG 1
//#define STBI_NO_PNG 1
//#define STBI_NO_BMP 1
#define STBI_NO_PSD 1
#define STBI_NO_TGA 1
#define STBI_NO_GIF 1
#define STBI_NO_HDR 1
#define STBI_NO_PIC 1
#define STBI_NO_PNM 1

#define STB_IMAGE_STATIC 1
#define STB_IMAGE_IMPLEMENTATION 1
#include "stb_image.h"

#define STB_IMAGE_WRITE_STATIC 1
#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#include "stb_image_write.h"

#include <cassert>
#include <cstring>

#include "Painter.hpp"

#ifndef DISABLE_QT
#include <QPixmap>
#include <QBuffer>
#endif

namespace FreeHeroes {

namespace {
char toHex(uint8_t c)
{
    return (c <= 9) ? '0' + c : 'a' + c - 10;
}

void write_func(void* context, void* data, int size)
{
    Mernel::ByteArrayHolder* holder  = reinterpret_cast<Mernel::ByteArrayHolder*>(context);
    auto                     oldSize = holder->size();
    holder->resize(holder->size() + size);
    memcpy(holder->data() + oldSize, data, size);
}

}

void Pixmap::loadPng(const Mernel::std_path& path)
{
    auto holder = Mernel::readFileIntoHolder(path);
    loadPngFromBuffer(holder);
}

void Pixmap::savePng(const Mernel::std_path& path) const
{
    if (isNull())
        throw std::runtime_error("pixmap is null.");

    Mernel::std_fs::create_directories(path.parent_path());

    Mernel::ByteArrayHolder holder;
    savePngToBuffer(holder);
    Mernel::writeFileFromHolder(path, holder);

    if (!Mernel::std_fs::exists(path))
        throw std::runtime_error("failed to write:" + Mernel::path2string(path));
}

void Pixmap::loadPngFromBuffer(const Mernel::ByteArrayHolder& holder)
{
#ifndef DISABLE_QT
    {
        QPixmap pix;
        if (pix.loadFromData(holder.data(), holder.size())) {
            fromQtPixmap(pix);
            return;
        }
    }
#endif

    int      w, h, channelsInFile;
    stbi_uc* data = stbi_load_from_memory(holder.data(), (int) holder.size(), &w, &h, &channelsInFile, 4);
    if (!data)
        throw std::runtime_error("Failed to decode png");
    m_size = { w, h };
    updateSize();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            auto* src = data + y * w * 4 + x * 4;
            auto& clr = get(x, y).m_color;
            clr.m_r   = src[0];
            clr.m_g   = src[1];
            clr.m_b   = src[2];
            clr.m_a   = src[3];
        }
    }
}

void Pixmap::savePngToBuffer(Mernel::ByteArrayHolder& holder) const
{
#ifndef DISABLE_QT
    {
        QByteArray buffer;
        QBuffer    buf(&buffer);
        QPixmap    pix = toQtPixmap();
        if (pix.save(&buf)) {
            holder.resize(buffer.size());
            memcpy(holder.data(), buffer.data(), buffer.size());
            return;
        }
    }
#endif

    int  comp   = 4;
    auto result = stbi_write_png_to_func(&write_func, &holder, m_size.m_width, m_size.m_height, comp, (const void*) m_pixels.data(), m_size.m_width * 4);
    if (result != 1)
        throw std::runtime_error("Failed to encode png");

    if (!holder.size())
        throw std::runtime_error("holder is empty afer write!");
}

void Pixmap::loadBmp(const Mernel::std_path& path)
{
    return loadPng(path); // Having separate function name, just in case we change implementation later.
}

Pixmap Pixmap::subframe(const PixmapPoint& offset, const PixmapSize& size) const
{
    Pixmap result(size);
    int    h = size.m_height;
    int    w = size.m_width;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            result.get(x, y) = get(x + offset.m_x, y + offset.m_y);
        }
    }
    return result;
}

Pixmap Pixmap::padToSize(const PixmapSize& size, const PixmapPoint& leftTop) const
{
    if (size == m_size)
        return *this;

    Pixmap  result(size);
    Painter p{ &result };
    p.drawPixmap(leftTop, *this);
    return result;
}

void Pixmap::flipVertical()
{
    Pixmap result(m_size);
    int    h = m_size.m_height;
    int    w = m_size.m_width;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            result.get(x, h - y - 1) = get(x, y);
        }
    }
    m_pixels = std::move(result.m_pixels);
}

std::string PixmapColor::toString() const noexcept
{
    std::string result;
    result.resize(8);
    result[0] = toHex(m_r / 16);
    result[1] = toHex(m_r % 16);
    result[2] = toHex(m_g / 16);
    result[3] = toHex(m_g % 16);
    result[4] = toHex(m_b / 16);
    result[5] = toHex(m_b % 16);
    result[6] = toHex(m_a / 16);
    result[7] = toHex(m_a % 16);
    return result;
}

void PixmapColor::fromString(const std::string& str) noexcept
{
    if (str.empty()) {
        *this = PixmapColor{};
        return;
    }
    const char* ptr = str.c_str();
    if (*ptr == '#')
        ptr++;
    const uint32_t pix = std::strtoul(ptr, nullptr, 16);

    if (str.size() >= 8) {
        m_a = pix & 0xFFU;
        m_b = (pix & (0xFFU << 8)) >> 8;
        m_g = (pix & (0xFFU << 16)) >> 16;
        m_r = (pix & (0xFFU << 24)) >> 24;
    } else {
        m_b = pix & 0xFFU;
        m_g = (pix & (0xFFU << 8)) >> 8;
        m_r = (pix & (0xFFU << 16)) >> 16;
        m_a = 255;
    }
}

#ifndef DISABLE_QT

QColor PixmapColor::toQColor() const noexcept
{
    return QColor(m_r, m_g, m_b, m_a);
}

QPoint PixmapPoint::toQPoint() const noexcept
{
    return QPoint(m_x, m_y);
}

QSize PixmapSize::toQSize() const noexcept
{
    return QSize(m_width, m_height);
}
QPixmap Pixmap::toQtPixmap() const
{
    int    h = m_size.m_height;
    int    w = m_size.m_width;
    QImage result(w, h, QImage::Format_RGBA8888);
    for (int y = 0; y < h; ++y) {
        uchar* line = result.scanLine(y);
        memcpy(line, m_pixels.data() + y * w, w * 4);
    }
    return QPixmap::fromImage(std::move(result));
}

void Pixmap::fromQtPixmap(const QPixmap& pixmap)
{
    int h  = pixmap.height();
    int w  = pixmap.width();
    m_size = { w, h };
    updateSize();
    QImage img = pixmap.toImage();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            auto&      colorDest = get(x, y).m_color;
            const auto colorSrc  = img.pixelColor(x, y);
            colorDest.m_r        = colorSrc.red();
            colorDest.m_g        = colorSrc.green();
            colorDest.m_b        = colorSrc.blue();
            colorDest.m_a        = colorSrc.alpha();
        }
    }
}
#endif

}
