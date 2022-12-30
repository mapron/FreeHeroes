/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

class QPixmap;

namespace FreeHeroes {

class ByteOrderDataStreamReader;
class ByteOrderDataStreamWriter;
class PropertyTree;

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
    Compression m_compression = Compression::Invalid;

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
    };
    struct Palette {
        std::vector<Pixel> m_table;

        void readBinary(ByteOrderDataStreamReader& stream);
        void writeBinary(ByteOrderDataStreamWriter& stream) const;
    };
    struct RLEItem {
        uint8_t              m_segmentType = 0;
        uint8_t              m_length      = 0;
        std::vector<uint8_t> m_raw;
        bool                 m_isRaw = false;
    };

    struct RLERow {
        std::vector<uint8_t> m_rle0;

        std::vector<RLEItem> m_items;

        void readBinary(ByteOrderDataStreamReader& stream, bool compressedLength, int width);
        void writeBinary(ByteOrderDataStreamWriter& stream, bool compressedLength, int width) const;
    };

    struct RLEData {
        std::vector<uint32_t> m_rle1offsets;
        std::vector<uint16_t> m_rle2offsets;
        std::vector<uint16_t> m_rle3offsets;

        std::vector<RLERow> m_rleRows;

        int32_t m_size = 0;
    };

    Palette m_palette;

    using PixelRow = std::vector<Pixel>;
    std::vector<PixelRow> m_rows;

    RLEData m_rleData;

    std::shared_ptr<QPixmap> m_pixmapQt;

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;

    void toJson(PropertyTree& data) const;
    void fromJson(const PropertyTree& data);
};

}
