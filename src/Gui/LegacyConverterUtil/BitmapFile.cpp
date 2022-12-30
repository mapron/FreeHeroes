/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BitmapFile.hpp"

#include "ByteOrderStream.hpp"
#include "PropertyTree.hpp"

#include "BitmapFileReflection.hpp"

#include "Reflection/PropertyTreeReader.hpp"
#include "Reflection/PropertyTreeWriter.hpp"

// for destructor of shared_ptr.
#include <QPixmap>

#include <sstream>

namespace FreeHeroes {
using namespace Core;

namespace {
char toHex(uint8_t c)
{
    return (c <= 9) ? '0' + c : 'a' + c - 10;
}

class OffsetCalculationReader {
    ByteOrderDataStreamReader& m_stream;
    int32_t                    m_base = 0;

public:
    OffsetCalculationReader(ByteOrderDataStreamReader& stream)
        : m_stream(stream)
    {
    }

    int32_t currentAbs() const
    {
        return m_stream.getBuffer().getOffsetRead();
    }
    void setCurrentAsBase()
    {
        m_base = currentAbs();
    }
    int32_t currentBased() const
    {
        return currentAbs() - m_base;
    }

    int32_t makeRelativeFromBased(int32_t offset) const
    {
        return offset - currentBased();
    }
};

}

std::string BitmapFile::Pixel::toString() const
{
    std::string result;
    result.resize(8);
    result[0] = toHex(m_r / 16);
    result[1] = toHex(m_r % 16);
    result[2] = toHex(m_g / 16);
    result[3] = toHex(m_g % 16);
    result[4] = toHex(m_b / 16);
    result[5] = toHex(m_b % 16);
    result[6] = toHex(m_alphaOrGray / 16);
    result[7] = toHex(m_alphaOrGray % 16);
    return result;
}

void BitmapFile::Pixel::fromString(const std::string& str)
{
    const uint32_t pix = std::stoul(str, nullptr, 16);
    m_alphaOrGray      = pix & 0xFFU;
    m_b                = (pix & (0xFFU << 8)) >> 8;
    m_g                = (pix & (0xFFU << 16)) >> 16;
    m_r                = (pix & (0xFFU << 24)) >> 24;
}

BitmapFile::BitmapFile()
{
}

BitmapFile::~BitmapFile()
{
    /*
    //load size raw pixels from data
    inline void Load(size_t size)
    {
        for (size_t i = 0; i < size; ++i) {
            uint8_t byte;
            m_ds >> byte;
            LoadPixel(byte);
        }
    }

    inline void Load(size_t size, uint8_t color)
    {
        for (size_t i = 0; i < size; ++i)
            LoadPixel(color);
    }

    inline void LoadPixel(uint8_t color)
    {
        assert(m_x < m_tmpImage.width());
        m_tmpImage.setPixel(m_x, m_y, color);
        m_x++;
        //  if (color == 5)
        //       m_flagColorUsed = true;
    }
    inline void EndLine()
    {
        m_y++;
        m_x = 0;
    }

    void LoadLine(bool compressedLength, const int baseOffset = 0)
    {
        
    }

    QPixmap LoadAll(int format)
    {
        const auto baseOffset = m_ds.device()->pos();
        switch (format) {
            
            default:
                qWarning() << "unknown def pix format=" << format;
                break;
        }
        QPixmap result = QPixmap::fromImage(m_tmpImage.convertToFormat(QImage::Format_RGBA8888));
        return result;
    }*/
}

void BitmapFile::readBinary(ByteOrderDataStreamReader& stream)
{
    if (m_compression == Compression::Invalid || m_pixFormat == PixFormat::Invalid)
        throw std::runtime_error("Need to specify bitmap Compression and PixFormat");

    if (m_compression >= Compression::RLE1 && m_compression <= Compression::RLE3) {
        stream.readBlock(m_rleBlob.data(), m_rleBlob.size());
        //m_rleData.m_rleRows.resize(m_height);
        //OffsetCalculationReader offsetCalc(stream);
        //offsetCalc.setCurrentAsBase();
        //m_rleData.m_offsetBaseAbs = offsetCalc.currentAbs();

        /*
        if (m_compression == Compression::RLE1) {
            //for each line we have offset of pixel data
            m_rleData.m_rle1offsets.resize(m_height);
            for (auto& offset : m_rleData.m_rle1offsets)
                stream >> offset;

            for (uint32_t y = 0; y < m_height; y++) {
                const auto rowIndex      = m_inverseRowOrder ? m_height - y - 1 : y;
                RLERow&    row           = m_rleData.m_rleRows[rowIndex];
                auto       currentOffset = m_rleData.m_offsetBaseAbs + m_rleData.m_rle1offsets[y];
                stream.getBuffer().setOffsetRead(currentOffset);
                row.readBinary(stream, false, m_width);
            }
        } else if (m_compression == Compression::RLE2) {
            {
                m_rleData.m_rle2offset1base = stream.readScalar<uint16_t>();
                m_rleData.m_rle2offset1rel  = offsetCalc.makeRelativeFromBased(m_rleData.m_rle2offset1base);
                stream.getBuffer().markRead(m_rleData.m_rle2offset1rel);
            }

            for (uint32_t y = 0; y < m_height; y++) {
                const auto rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
                RLERow&    row      = m_rleData.m_rleRows[rowIndex];

                OffsetCalculationReader r2(stream);
                row.readBinary(stream, true, m_width);
                row.m_rleSize = r2.currentBased();
            }
        } else if (m_compression == Compression::RLE3) {
            for (uint32_t y = 0; y < m_height; y++) {
                const auto rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
                RLERow&    row      = m_rleData.m_rleRows[rowIndex];

                row.m_currentBased1 = offsetCalc.currentBased();
                {
                    row.m_rle3offset1base = y * 2 * (m_width / 32);
                    row.m_rle3offset1rel  = offsetCalc.makeRelativeFromBased(row.m_rle3offset1base);
                    stream.getBuffer().markRead(row.m_rle3offset1rel);
                }

                row.m_currentBased2 = offsetCalc.currentBased();
                {
                    row.m_rle3offset2base = stream.readScalar<uint16_t>();
                    row.m_rle3offset2rel  = offsetCalc.makeRelativeFromBased(row.m_rle3offset2base);
                    stream.getBuffer().markRead(row.m_rle3offset2rel);
                }

                OffsetCalculationReader r2(stream);
                r2.setCurrentAsBase();
                row.readBinary(stream, true, m_width);
                row.m_rleSize = r2.currentBased();
            }
        }*/

        return;
    }

    m_rows.resize(m_height);

    for (uint32_t y = 0; y < m_height; y++) {
        const auto rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
        auto&      row      = m_rows[rowIndex];
        row.resize(m_width);
        for (uint32_t x = 0; x < m_width; x++) {
            Pixel& pix = row[x];

            // clang-format off
            switch(m_pixFormat) {
                case PixFormat::Gray: stream >> pix.m_alphaOrGray; break;
                case PixFormat::BGRA: stream >> pix.m_b >> pix.m_g >> pix.m_r >> pix.m_alphaOrGray; break;
                case PixFormat::RGBA: stream >> pix.m_r >> pix.m_g >> pix.m_b >> pix.m_alphaOrGray; break;
                case PixFormat::ARGB: stream >> pix.m_alphaOrGray >> pix.m_r >> pix.m_g >> pix.m_b; break;
                case PixFormat::ABGR: stream >> pix.m_alphaOrGray >> pix.m_b >> pix.m_g >> pix.m_r; break;
                case PixFormat::Invalid:
                break;
            }
            // clang-format on
        }
    }
}

void BitmapFile::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    if (m_compression == Compression::Invalid || m_pixFormat == PixFormat::Invalid)
        throw std::runtime_error("Need to specify bitmap Compression and PixFormat");

    if (m_compression >= Compression::RLE1 && m_compression <= Compression::RLE3) {
        stream.writeBlock(m_rleBlob.data(), m_rleBlob.size());
        /*
        const auto baseOffset = stream.getBuffer().getOffsetWrite();
        assert(m_rleData.m_offsetBaseAbs == baseOffset);

        if (m_compression == Compression::RLE1) {
            //for each line we have offset of pixel data
            for (const auto& offset : m_rleData.m_rle1offsets)
                stream << offset;

            for (uint32_t y = 0; y < m_height; y++) {
                const auto    rowIndex      = m_inverseRowOrder ? m_height - y - 1 : y;
                const RLERow& row           = m_rleData.m_rleRows[rowIndex];
                auto          currentOffset = baseOffset + m_rleData.m_rle1offsets[y];
                stream.getBuffer().setOffsetWrite(currentOffset);
                stream.getBuffer().extendWriteToCurrentOffset();
                row.writeBinary(stream, false, m_width);
            }
        } else if (m_compression == Compression::RLE2) {
            stream << static_cast<uint16_t>(m_rleData.m_rle2offset1base);
            stream.getBuffer().markWrite(m_rleData.m_rle2offset1rel);
            stream.getBuffer().extendWriteToCurrentOffset();

            for (uint32_t y = 0; y < m_height; y++) {
                const auto    rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
                const RLERow& row      = m_rleData.m_rleRows[rowIndex];
                row.writeBinary(stream, true, m_width);
            }
        } else if (m_compression == Compression::RLE3) {
            for (uint32_t y = 0; y < m_height; y++) {
                const auto    rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
                const RLERow& row      = m_rleData.m_rleRows[rowIndex];

                stream << static_cast<uint16_t>(row.m_rle3offset2base);
                stream.getBuffer().markWrite(row.m_rle3offset2rel);
                stream.getBuffer().extendWriteToCurrentOffset();

                row.writeBinary(stream, true, m_width);
            }
        }*/

        return;
    }

    for (uint32_t y = 0; y < m_height; y++) {
        const auto  rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
        const auto& row      = m_rows[rowIndex];
        for (uint32_t x = 0; x < m_width; x++) {
            const Pixel& pix = row[x];

            // clang-format off
            switch(m_pixFormat) {
                case PixFormat::Gray: stream << pix.m_alphaOrGray; break;
                case PixFormat::BGRA: stream << pix.m_b << pix.m_g << pix.m_r << pix.m_alphaOrGray; break;
                case PixFormat::RGBA: stream << pix.m_r << pix.m_g << pix.m_b << pix.m_alphaOrGray; break;
                case PixFormat::ARGB: stream << pix.m_alphaOrGray << pix.m_r << pix.m_g << pix.m_b; break;
                case PixFormat::ABGR: stream << pix.m_alphaOrGray << pix.m_b << pix.m_g << pix.m_r; break;
                case PixFormat::Invalid:
                break;
            }
            // clang-format on
        }
    }
}

void BitmapFile::toJson(PropertyTree& data) const
{
    Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void BitmapFile::fromJson(const PropertyTree& data)
{
    Reflection::PropertyTreeReader reader;
    reader.jsonToValue(data, *this);
}

void BitmapFile::Palette::readBinary(ByteOrderDataStreamReader& stream)
{
    m_table.resize(256);
    for (Pixel& pix : m_table) {
        stream >> pix.m_r >> pix.m_g >> pix.m_b;
        pix.m_alphaOrGray = 255;
    }
}

void BitmapFile::Palette::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    for (const Pixel& pix : m_table) {
        stream << pix.m_r << pix.m_g << pix.m_b;
        if (pix.m_alphaOrGray != 255)
            throw std::runtime_error("While writing palette to binary, alpha must be trivial (255)");
    }
}

void BitmapFile::RLERow::readBinary(ByteOrderDataStreamReader& stream, bool compressedLength, int width)
{
    int currentLength = 0;

    while (currentLength < width) {
        RLEItem item;
        item.m_segmentType = stream.readScalar<uint8_t>();
        uint8_t  code;
        uint32_t length;
        if (compressedLength) {
            code         = item.m_segmentType / 32;
            length       = (item.m_segmentType & 31) + 1;
            item.m_isRaw = code == 7;
        } else {
            stream >> item.m_length;
            code         = item.m_segmentType;
            length       = item.m_length + 1;
            item.m_isRaw = item.m_segmentType == 0xFF;
        }

        if (item.m_isRaw) {
            item.m_raw.resize(length);
            for (size_t i = 0; i < item.m_raw.size(); ++i) {
                stream >> item.m_raw[i];
            }
        }
        (void) code;

        currentLength += length;
        m_items.push_back(item);
    }
}

void BitmapFile::RLERow::writeBinary(ByteOrderDataStreamWriter& stream, bool compressedLength, int width) const
{
    for (const RLEItem& item : m_items) {
        stream << item.m_segmentType;
        if (!compressedLength)
            stream << item.m_length;
        if (item.m_isRaw) {
            for (size_t i = 0; i < item.m_raw.size(); ++i) {
                stream << item.m_raw[i];
            }
        }
    }
}

}
