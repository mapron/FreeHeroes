/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtils.hpp"

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <compare>
#include <map>

class QPixmap;

namespace FreeHeroes {

class ByteOrderDataStreamReader;
class ByteOrderDataStreamWriter;
class PropertyTree;
class ByteArrayHolder;

class BitmapFile {
public:
    BitmapFile();
    ~BitmapFile();

    enum class Compression
    {
        Invalid,
        None,
        RLE1,
        RLE2,
        RLE3,
    };
    Compression m_compression         = Compression::Invalid;
    Compression m_compressionOriginal = Compression::Invalid;

    enum class PixFormat
    {
        Invalid,
        Gray,
        RGBA,
        BGRA,
        ARGB,
        ABGR,
    };
    PixFormat m_pixFormat = PixFormat::Invalid;

    uint32_t m_width           = 0;
    uint32_t m_height          = 0;
    bool     m_inverseRowOrder = false;

    struct Pixel {
        uint8_t m_r           = 0;
        uint8_t m_g           = 0;
        uint8_t m_b           = 0;
        uint8_t m_alphaOrGray = 0;

        std::string toString() const;
        void        fromString(const std::string& str);

        auto operator<=>(const Pixel&) const noexcept = default;
    };
    struct Palette {
        std::vector<Pixel>       m_table;
        std::map<Pixel, size_t>  m_counter;
        std::map<Pixel, uint8_t> m_index;

        void readBinary(ByteOrderDataStreamReader& stream);
        void writeBinary(ByteOrderDataStreamWriter& stream) const;
    };

    struct RLEItemRaw {
        std::vector<uint8_t> m_bytes;
    };
    struct RLEItemNorm {
        uint8_t m_value  = 0;
        int     m_length = 0;
    };

    struct RLEItem {
        bool m_isRaw = false;

        bool m_isCompressedLength = true;

        RLEItemRaw  m_raw;
        RLEItemNorm m_norm;

        size_t getByteSize() const
        {
            if (m_isCompressedLength)
                return m_isRaw ? m_raw.m_bytes.size() + 1 : 1;
            else
                return m_isRaw ? m_raw.m_bytes.size() + 2 : 2;
        }
    };

    struct RLERow {
        std::vector<RLEItem> m_items;

        int m_width = 0;

        bool m_isCompressedLength = true; // not serialized

        void readBinary(ByteOrderDataStreamReader& stream);
        void writeBinary(ByteOrderDataStreamWriter& stream) const;
    };

    struct RLEData {
        std::vector<uint32_t> m_rle1offsets;
        std::vector<uint16_t> m_rle2offsets;
        std::vector<uint16_t> m_rle3offsets;

        std::vector<RLERow> m_rleRows;

        uint32_t m_originalSize = 0;
    };

    Palette m_palette;

    using PixelRow = std::vector<Pixel>;
    std::vector<PixelRow> m_rows;

    RLEData m_rleData;

    std::shared_ptr<QPixmap> m_pixmapQt;

    void readFromBlob(ByteArrayHolder& blob);
    void writeToBlob(ByteArrayHolder& blob) const;

    void toJson(PropertyTree& data) const;
    void fromJson(const PropertyTree& data);

    void uncompress();
    void compressToOriginal();

    void unpackPalette(const Palette& pal, const Palette& palExtended);
    void countPalette(Palette& pal);
    void packPalette(const Palette& pal);

    void toPixmapQt();
    void fromPixmapQt();

    void loadPixmapQt(const Core::std_path& filename);
    void savePixmapQt(const Core::std_path& filename) const;
};

}
