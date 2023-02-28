/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BitmapFile.hpp"

#include "MernelPlatform/ByteOrderStream.hpp"
#include "MernelPlatform/PropertyTree.hpp"

#include "FsUtilsQt.hpp"

#include "BitmapFileReflection.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"
#include "MernelReflection/PropertyTreeWriter.hpp"

// for destructor of shared_ptr.
#include <QPixmap>
#include <QImage>

#include <sstream>
#include <iostream>

namespace FreeHeroes {
using namespace Mernel;

namespace {
char toHex(uint8_t c)
{
    return (c <= 9) ? '0' + c : 'a' + c - 10;
}

}

class RLEncoder {
public:
    const BitmapFile::Pixel* m_data;
    size_t                   m_size = 0;
    size_t                   m_pos  = 0;

    struct Chunk {
        uint8_t m_value = 0;
        size_t  m_size  = 0;
    };
    int m_maximumRLvalue = 255;

    std::vector<Chunk> m_chunks;

    std::vector<BitmapFile::RLEItem> m_rleItems;

    Chunk m_currentChunk;

    void setBytes(const BitmapFile::Pixel* data, size_t size)
    {
        m_data = data;
        m_size = size;
        m_pos  = 1;

        m_currentChunk.m_value = data[0].m_alphaOrGray;
        m_currentChunk.m_size  = 1;
    }

    void findAllChunks()
    {
        while (m_pos < m_size) {
            uint8_t value = m_data[m_pos].m_alphaOrGray;
            if (value == m_currentChunk.m_value && value <= m_maximumRLvalue) {
                m_currentChunk.m_size++;
            } else {
                m_chunks.push_back(m_currentChunk);
                m_currentChunk.m_value = value;
                m_currentChunk.m_size  = 1;
            }
            m_pos++;
        }
        m_chunks.push_back(m_currentChunk);
    }

    void makeItems(bool compressedLength)
    {
        BitmapFile::RLEItem currentRaw;
        currentRaw.m_isCompressedLength = compressedLength;
        currentRaw.m_isRaw              = true;

        BitmapFile::RLEItem norm;
        norm.m_isCompressedLength = compressedLength;
        norm.m_isRaw              = false;

        auto pushRawIfNeeded = [&currentRaw, this] {
            if (currentRaw.m_raw.m_bytes.empty())
                return;
            m_rleItems.push_back(currentRaw);
            currentRaw.m_raw.m_bytes.clear();
        };

        for (const auto& chunk : m_chunks) {
            if (chunk.m_size > 1) {
                pushRawIfNeeded();
                norm.m_norm.m_length = chunk.m_size;
                norm.m_norm.m_value  = chunk.m_value;
                m_rleItems.push_back(norm);
            } else if (chunk.m_value <= m_maximumRLvalue) {
                if (false && currentRaw.m_raw.m_bytes.size() > 2) {
                    currentRaw.m_raw.m_bytes.push_back(chunk.m_value);
                } else {
                    pushRawIfNeeded();
                    norm.m_norm.m_length = chunk.m_size;
                    norm.m_norm.m_value  = chunk.m_value;
                    m_rleItems.push_back(norm);
                }
            } else {
                currentRaw.m_raw.m_bytes.push_back(chunk.m_value);
            }
        }
        pushRawIfNeeded();
    }

    size_t getByteSize() const
    {
        size_t result = 0;
        for (const auto& item : m_rleItems) {
            result += item.getByteSize();
        }
        return result;
    }
};

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

void BitmapFile::readFromBlob(ByteArrayHolder& blob)
{
    if (m_compression == Compression::Invalid || m_pixFormat == PixFormat::Invalid)
        throw std::runtime_error("Need to specify bitmap Compression and PixFormat");

    ByteOrderBuffer           bobuffer(blob);
    ByteOrderDataStreamReader stream(bobuffer, ByteOrderDataStream::s_littleEndian);

    if (m_compression >= Compression::RLE1 && m_compression <= Compression::RLE3) {
        m_rleData.m_rleRows.resize(m_height);
        for (RLERow& row : m_rleData.m_rleRows)
            row.m_width = m_width;
        if (m_compression == Compression::RLE1) {
            //for each line we have offset of pixel data
            m_rleData.m_rle1offsets.resize(m_height);
            for (auto& offset : m_rleData.m_rle1offsets)
                stream >> offset;

            [[maybe_unused]] const auto bufSize = blob.size();

            for (uint32_t y = 0; y < m_height; y++) {
                const auto rowIndex      = m_inverseRowOrder ? m_height - y - 1 : y;
                RLERow&    row           = m_rleData.m_rleRows[rowIndex];
                auto       currentOffset = m_rleData.m_rle1offsets[y];
                assert(currentOffset < bufSize);
                stream.getBuffer().setOffsetRead(currentOffset);
                row.m_isCompressedLength = false;
                row.readBinary(stream);
            }
        } else if (m_compression == Compression::RLE2) {
            m_rleData.m_rle2offsets.resize(m_height);
            for (auto& offset : m_rleData.m_rle2offsets)
                stream >> offset;
            // some buggy defs has one extra unused offset
            if (m_rleData.m_rle2offsets[0] != stream.getBuffer().getOffsetRead()) {
                m_rleData.m_rle2offsets.push_back(stream.readScalar<uint16_t>());
            }

            for (uint32_t y = 0; y < m_height; y++) {
                const auto rowIndex      = m_inverseRowOrder ? m_height - y - 1 : y;
                RLERow&    row           = m_rleData.m_rleRows[rowIndex];
                row.m_isCompressedLength = true;
                row.readBinary(stream);
            }
        } else if (m_compression == Compression::RLE3) {
            m_rleData.m_rle3offsets.resize(m_height * ((m_width + 31) / 32));
            for (auto& offset : m_rleData.m_rle3offsets)
                stream >> offset;
            // some buggy defs (avlskul0) has one extra unused offset
            if (m_rleData.m_rle3offsets[0] != stream.getBuffer().getOffsetRead()) {
                m_rleData.m_rle3offsets.push_back(stream.readScalar<uint16_t>());
            }

            for (uint32_t y = 0; y < m_height; y++) {
                const auto rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
                RLERow&    row      = m_rleData.m_rleRows[rowIndex];

                auto rle3offset2base = m_rleData.m_rle3offsets[y * ((m_width + 31) / 32)];
                stream.getBuffer().setOffsetRead(rle3offset2base);

                row.m_isCompressedLength = true;
                row.readBinary(stream);
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

void BitmapFile::writeToBlob(ByteArrayHolder& blob) const
{
    if (m_compression == Compression::Invalid || m_pixFormat == PixFormat::Invalid)
        throw std::runtime_error("Need to specify bitmap Compression and PixFormat");

    ByteOrderBuffer bobuffer(blob);

    ByteOrderDataStreamWriter stream(bobuffer, ByteOrderDataStream::s_littleEndian);

    if (m_compression >= Compression::RLE1 && m_compression <= Compression::RLE3) {
        // this is for RLE round-trip only.
        bobuffer.setSize(m_rleData.m_originalSize);
        if (m_compression == Compression::RLE1) {
            //for each line we have offset of pixel data
            for (const auto& offset : m_rleData.m_rle1offsets)
                stream << offset;

            for (uint32_t y = 0; y < m_height; y++) {
                const auto    rowIndex      = m_inverseRowOrder ? m_height - y - 1 : y;
                const RLERow& row           = m_rleData.m_rleRows[rowIndex];
                auto          currentOffset = m_rleData.m_rle1offsets[y];
                stream.getBuffer().setOffsetWrite(currentOffset);
                row.writeBinary(stream);
            }
        } else if (m_compression == Compression::RLE2) {
            for (const auto& offset : m_rleData.m_rle2offsets)
                stream << offset;

            for (uint32_t y = 0; y < m_height; y++) {
                const auto    rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
                const RLERow& row      = m_rleData.m_rleRows[rowIndex];
                row.writeBinary(stream);
            }
        } else if (m_compression == Compression::RLE3) {
            for (auto& offset : m_rleData.m_rle3offsets)
                stream << offset;
            for (uint32_t y = 0; y < m_height; y++) {
                const auto    rowIndex = m_inverseRowOrder ? m_height - y - 1 : y;
                const RLERow& row      = m_rleData.m_rleRows[rowIndex];

                const auto rle3offset2base = m_rleData.m_rle3offsets[y * (m_width / 32)];
                stream.getBuffer().setOffsetWrite(rle3offset2base);

                row.writeBinary(stream);
            }
        }

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

void BitmapFile::uncompress()
{
    if (m_compression == Compression::None)
        return;
    m_compression = Compression::None;
    assert(m_pixFormat == PixFormat::Gray);

    m_rows.resize(m_height);
    for (uint32_t y = 0; y < m_height; y++) {
        auto& row = m_rows[y];
        row.resize(m_width);
        const auto& rleRow = m_rleData.m_rleRows[y];

        uint32_t x = 0;
        for (const auto& item : rleRow.m_items) {
            if (item.m_isRaw) {
                for (const auto& byte : item.m_raw.m_bytes) {
                    row[x].m_alphaOrGray = byte;
                    x++;
                }
            } else {
                for (int i = 0; i < item.m_norm.m_length; ++i) {
                    row[x].m_alphaOrGray = item.m_norm.m_value;
                    x++;
                }
            }
        }
    }
    const auto oldSize       = m_rleData.m_originalSize;
    m_rleData                = {};
    m_rleData.m_originalSize = oldSize;
}

void BitmapFile::compressToOriginal()
{
    if (m_compressionOriginal == Compression::None)
        return;
    if (m_compression == m_compressionOriginal)
        return;

    m_compression = m_compressionOriginal;

    if (m_compression == Compression::RLE1) {
        //for each line we have offset of pixel data
        m_rleData.m_rle1offsets.resize(m_height);
    } else if (m_compression == Compression::RLE2) {
        m_rleData.m_rle2offsets.resize(m_height);
    } else if (m_compression == Compression::RLE3) {
        m_rleData.m_rle3offsets.resize(m_height * ((m_width + 31) / 32));
    }
    size_t offsetsSizeBytes = m_rleData.m_rle3offsets.size() * 2;

    size_t offset = offsetsSizeBytes;

    m_rleData.m_rleRows.resize(m_height);

    for (uint32_t y = 0; y < m_height; y++) {
        const auto& row             = m_rows[y];
        auto&       rleRow          = m_rleData.m_rleRows[y];
        rleRow.m_isCompressedLength = true;
        rleRow.m_width              = m_width;

        for (size_t wpart = 0; wpart < (m_width + 31) / 32; wpart++) {
            int xOffset = wpart * 32;
            int xRemain = m_width - xOffset;
            int width   = std::min(xRemain, 32);

            RLEncoder enc;
            enc.m_maximumRLvalue = 6; // 0..6 - RL , 7 = raw. 3 bits.
            enc.setBytes(row.data() + xOffset, width);
            enc.findAllChunks();
            enc.makeItems(rleRow.m_isCompressedLength);

            rleRow.m_items.insert(rleRow.m_items.end(), enc.m_rleItems.cbegin(), enc.m_rleItems.cend());

            if (m_compression == Compression::RLE1) {
            } else if (m_compression == Compression::RLE2) {
            } else if (m_compression == Compression::RLE3) {
                m_rleData.m_rle3offsets[y * (m_width / 32) + wpart] = offset;
            }

            offset += enc.getByteSize();
        }
    }

    m_rows.clear();
}

void BitmapFile::unpackPalette(const Palette& pal, const Palette& palExtended)
{
    assert(m_pixFormat == PixFormat::Gray);
    m_pixFormat = PixFormat::RGBA;
    for (uint32_t y = 0; y < m_height; y++) {
        auto& row = m_rows[y];
        for (uint32_t x = 0; x < m_width; x++) {
            Pixel& pix = row[x];
            auto   idx = pix.m_alphaOrGray;
            if (idx < palExtended.m_table.size())
                pix = palExtended.m_table[idx];
            else
                pix = pal.m_table[idx];
        }
    }
}

void BitmapFile::countPalette(Palette& pal)
{
    for (uint32_t y = 0; y < m_height; y++) {
        auto& row = m_rows[y];
        for (uint32_t x = 0; x < m_width; x++) {
            Pixel& pix = row[x];
            pal.m_counter[pix]++;
        }
    }
}

void BitmapFile::packPalette(const Palette& pal)
{
    assert(m_pixFormat != PixFormat::Gray);
    m_pixFormat = PixFormat::Gray;
    for (uint32_t y = 0; y < m_height; y++) {
        auto& row = m_rows[y];
        for (uint32_t x = 0; x < m_width; x++) {
            Pixel& pix        = row[x];
            auto   idx        = pal.m_index.at(pix);
            pix.m_alphaOrGray = idx;
        }
    }
}

void BitmapFile::toPixmapQt()
{
    if (m_compression != Compression::None)
        throw std::runtime_error("You must uncompress RLE first.");

    if (m_pixmapQt)
        return;

    QImage image(m_width, m_height, QImage::Format_RGBA8888);
    for (uint32_t y = 0; y < m_height; y++) {
        const auto& row = m_rows[y];
        for (uint32_t x = 0; x < m_width; x++) {
            const Pixel& pix = row[x];
            if (m_pixFormat == PixFormat::Gray) {
                image.setPixelColor(x, y, QColor(pix.m_alphaOrGray, pix.m_alphaOrGray, pix.m_alphaOrGray));
            } else {
                image.setPixelColor(x, y, QColor(pix.m_r, pix.m_g, pix.m_b, pix.m_alphaOrGray));
            }
        }
    }

    m_pixmapQt = std::make_shared<QPixmap>(QPixmap::fromImage(std::move(image)));
    assert(m_pixmapQt->width() == (int) m_width);
    assert(m_pixmapQt->height() == (int) m_height);
    m_rows.clear();
}

void BitmapFile::fromPixmapQt()
{
    if (m_compression != Compression::None)
        throw std::runtime_error("Cannot go to RLE compression from Qt Pixmap.");

    if (!m_pixmapQt)
        throw std::runtime_error("Qt pixmap is missing.");

    const QImage image = m_pixmapQt->toImage();
    if (image.width() != (int) m_width || image.height() != (int) m_height)
        throw std::runtime_error("Qt pixmap has wrong dimensions.");

    m_rows.resize(m_height);
    for (uint32_t y = 0; y < m_height; y++) {
        auto& row = m_rows[y];
        row.resize(m_width);
        for (uint32_t x = 0; x < m_width; x++) {
            Pixel&       pix   = row[x];
            const QColor color = image.pixelColor(x, y);
            if (m_pixFormat == PixFormat::Gray) {
                pix.m_alphaOrGray = color.red();
            } else {
                pix.m_r           = color.red();
                pix.m_g           = color.green();
                pix.m_b           = color.blue();
                pix.m_alphaOrGray = color.alpha();
            }
        }
    }
    m_pixmapQt.reset();
}

void BitmapFile::loadPixmapQt(const Mernel::std_path& filename)
{
    m_pixmapQt = std::make_shared<QPixmap>(Gui::stdPath2QString(filename));

    if (m_pixmapQt->width() != (int) m_width || m_pixmapQt->height() != (int) m_height)
        throw std::runtime_error("Qt pixmap has wrong dimensions.");
}

void BitmapFile::savePixmapQt(const Mernel::std_path& filename) const
{
    if (!m_pixmapQt)
        throw std::runtime_error("Qt pixmap is missing.");
    Mernel::std_fs::create_directories(filename.parent_path());
    m_pixmapQt->save(Gui::stdPath2QString(filename));
    assert(Mernel::std_fs::exists(filename));
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

void BitmapFile::RLERow::readBinary(ByteOrderDataStreamReader& stream)
{
    int currentLength = 0;

    while (currentLength < m_width) {
        RLEItem item;
        item.m_isCompressedLength = m_isCompressedLength;

        uint8_t segmentType  = stream.readScalar<uint8_t>();
        uint8_t code         = 0;
        int     lengthActual = 0;

        if (m_isCompressedLength) {
            code         = segmentType / 32;
            lengthActual = (segmentType & 31) + 1;
            item.m_isRaw = code == 7;
        } else {
            uint8_t length = stream.readScalar<uint8_t>();

            code         = segmentType;
            lengthActual = length + 1;
            item.m_isRaw = segmentType == 0xFFU;
        }

        if (item.m_isRaw) {
            item.m_raw.m_bytes.resize(lengthActual);
            for (auto& byte : item.m_raw.m_bytes) {
                stream >> byte;
            }
        } else {
            item.m_norm.m_length = lengthActual;
            item.m_norm.m_value  = code;
        }

        currentLength += lengthActual;
        m_items.push_back(item);
    }
}

void BitmapFile::RLERow::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    for (const RLEItem& item : m_items) {
        uint8_t segmentType = 0;
        uint8_t length      = 0;
        if (item.m_isCompressedLength) {
            if (item.m_isRaw) {
                segmentType = 7 * 32 + (item.m_raw.m_bytes.size() - 1);
            } else {
                segmentType |= item.m_norm.m_value * 32;
                segmentType |= item.m_norm.m_length - 1;
            }

        } else {
            if (item.m_isRaw) {
                segmentType = 0xFFU;
                length      = (item.m_raw.m_bytes.size() - 1);
            } else {
                segmentType = item.m_norm.m_value;
                length      = item.m_norm.m_length - 1;
            }
        }

        stream << segmentType;
        if (!item.m_isCompressedLength)
            stream << length;
        if (item.m_isRaw) {
            for (auto& byte : item.m_raw.m_bytes) {
                stream << byte;
            }
        }
    }
}

}
