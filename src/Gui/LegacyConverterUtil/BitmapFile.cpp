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
}

void BitmapFile::readBinary(ByteOrderDataStreamReader& stream)
{
    if (m_compression == Compression::Invalid || m_pixFormat == PixFormat::Invalid)
        throw std::runtime_error("Need to specify bitmap Compression and PixFormat");

    if (m_compression >= Compression::RLE1 && m_compression <= Compression::RLE3) {
        m_rleData.m_rleRows.resize(m_height);

        ByteArrayHolder rleBlob;
        rleBlob.resize(m_rleData.m_size);
        stream.readBlock(rleBlob.data(), rleBlob.size());
        ByteOrderBuffer           bobuffer(rleBlob);
        ByteOrderDataStreamReader rleStream(bobuffer, ByteOrderDataStream::LITTLE_ENDIAN);

        if (m_compression == Compression::RLE1) {
            //for each line we have offset of pixel data
            m_rleData.m_rle1offsets.resize(m_height);
            for (auto& offset : m_rleData.m_rle1offsets)
                rleStream >> offset;

            for (uint32_t y = 0; y < m_height; y++) {
                const auto rowIndex      = m_inverseRowOrder ? m_height - y - 1 : y;
                RLERow&    row           = m_rleData.m_rleRows[rowIndex];
                auto       currentOffset = m_rleData.m_rle1offsets[y];
                rleStream.getBuffer().setOffsetRead(currentOffset);
                row.readBinary(rleStream, false, m_width);
            }
        } else if (m_compression == Compression::RLE2) {
            m_rleData.m_rle2offsets.resize(m_height);
            for (auto& offset : m_rleData.m_rle2offsets)
                rleStream >> offset;
            if (m_rleData.m_rle2offsets[0] != rleStream.getBuffer().getOffsetRead()) {
                m_rleData.m_rle2offsets.push_back(rleStream.readScalar<uint16_t>());
            }

            for (uint32_t y = 0; y < m_height; y++) {
                const auto rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
                RLERow&    row      = m_rleData.m_rleRows[rowIndex];

                row.readBinary(rleStream, true, m_width);
            }
        } else if (m_compression == Compression::RLE3) {
            m_rleData.m_rle3offsets.resize(m_height * (m_width / 32));
            for (auto& offset : m_rleData.m_rle3offsets)
                rleStream >> offset;
            if (m_rleData.m_rle3offsets[0] != rleStream.getBuffer().getOffsetRead()) {
                m_rleData.m_rle3offsets.push_back(rleStream.readScalar<uint16_t>());
            }

            for (uint32_t y = 0; y < m_height; y++) {
                const auto rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
                RLERow&    row      = m_rleData.m_rleRows[rowIndex];
                {
                    auto rle3offset2base = m_rleData.m_rle3offsets[y * (m_width / 32)];
                    rleStream.getBuffer().setOffsetRead(rle3offset2base);
                }

                row.readBinary(rleStream, true, m_width);
            }
        }

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
        ByteArrayHolder rleBlob;
        rleBlob.resize(m_rleData.m_size);

        ByteOrderBuffer bobuffer(rleBlob);
        bobuffer.setResizeEnabled(false);
        ByteOrderDataStreamWriter rleStream(bobuffer, ByteOrderDataStream::LITTLE_ENDIAN);

        if (m_compression == Compression::RLE1) {
            //for each line we have offset of pixel data
            for (const auto& offset : m_rleData.m_rle1offsets)
                rleStream << offset;

            for (uint32_t y = 0; y < m_height; y++) {
                const auto    rowIndex      = m_inverseRowOrder ? m_height - y - 1 : y;
                const RLERow& row           = m_rleData.m_rleRows[rowIndex];
                auto          currentOffset = m_rleData.m_rle1offsets[y];
                rleStream.getBuffer().setOffsetWrite(currentOffset);
                row.writeBinary(rleStream, false, m_width);
            }
        } else if (m_compression == Compression::RLE2) {
            for (const auto& offset : m_rleData.m_rle2offsets)
                rleStream << offset;

            for (uint32_t y = 0; y < m_height; y++) {
                const auto    rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
                const RLERow& row      = m_rleData.m_rleRows[rowIndex];
                row.writeBinary(rleStream, true, m_width);
            }
        } else if (m_compression == Compression::RLE3) {
            for (auto& offset : m_rleData.m_rle3offsets)
                rleStream << offset;
            for (uint32_t y = 0; y < m_height; y++) {
                const auto    rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
                const RLERow& row      = m_rleData.m_rleRows[rowIndex];

                {
                    auto rle3offset2base = m_rleData.m_rle3offsets[y * (m_width / 32)];
                    rleStream.getBuffer().setOffsetWrite(rle3offset2base);
                }

                row.writeBinary(rleStream, true, m_width);
            }
        }
        stream.writeBlock(rleBlob.data(), rleBlob.size());

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
