/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "Archive.hpp"

#include "FileFormatJson.hpp"
#include "FileIOUtils.hpp"
#include "StringUtils.hpp"

#include "Reflection/PropertyTreeReader.hpp"
#include "Reflection/PropertyTreeWriter.hpp"

#include "ArchiveReflection.hpp"

#include <iostream>

namespace FreeHeroes {
using namespace Core;

namespace {
const std_path g_indexFileName = "archive_index.fh.json";

constexpr const std::array<uint8_t, 4> g_lodSignature{ { 0x4C, 0x4F, 0x44, 0x00 } };  // 'LOD\0'
constexpr const std::array<uint8_t, 4> g_hdatSignature{ { 0x48, 0x44, 0x41, 0x54 } }; // 'HDAT'
constexpr const size_t                 g_strSize = 16;

constexpr const std::string_view g_hdatChapterSeparator{ "\r\n=============================\r\n" };
}

void Archive::detectFormat(const std_path& path, ByteOrderDataStreamReader& stream)
{
    std::array<uint8_t, 4> signature;
    stream >> signature;
    if (signature == g_hdatSignature) {
        m_format = BinaryFormat::HDAT;
        return;
    }
    if (signature == g_lodSignature) {
        m_format = BinaryFormat::LOD;
        return;
    }
    std::string ext = path2string(path.extension());
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    if (ext == ".lod") {
        m_format = BinaryFormat::LOD;
    } else if (ext == ".snd") {
        m_format = BinaryFormat::SND;
    } else if (ext == ".vid") {
        m_format = BinaryFormat::VID;
    } else {
        throw std::runtime_error("Filed to detect binary format for:" + path2string(path));
    }
}

void Archive::readBinary(ByteOrderDataStreamReader& stream)
{
    switch (m_format) {
        case BinaryFormat::LOD:
            readBinaryLOD(stream);
            break;
        case BinaryFormat::HDAT:
            readBinaryHDAT(stream);
            break;
        case BinaryFormat::SND:
            readBinarySND(stream);
            break;
        case BinaryFormat::VID:
            readBinaryVID(stream);
            break;
        default:
            throw std::runtime_error("Invalid binary format is set");
    }
    m_isBinary = true;
}

void Archive::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    if (!m_isBinary)
        throw std::runtime_error("Need to convertToBinary() first!");

    switch (m_format) {
        case BinaryFormat::LOD:
            writeBinaryLOD(stream);
            break;
        case BinaryFormat::HDAT:
            writeBinaryHDAT(stream);
            break;
        case BinaryFormat::SND:
            writeBinarySND(stream);
            break;
        case BinaryFormat::VID:
            writeBinaryVID(stream);
            break;
        default:
            throw std::runtime_error("Invalid binary format is set");
    }
}

void Archive::saveToFolder(const std_path& path, bool skipExisting) const
{
    if (m_isBinary)
        throw std::runtime_error("Need to convertFromBinary() first!");

    std_fs::create_directories(path);
    const auto jsonFilename = path / g_indexFileName;

    PropertyTree                   data;
    Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);

    std::string buffer = writeJsonToBufferThrow(data, true);
    writeFileFromBufferThrow(jsonFilename, buffer);

    for (const Record& rec : m_records) {
        const auto out = path / string2path(rec.m_filename);
        if (skipExisting && std_fs::exists(out))
            continue;
        writeFileFromHolderThrow(out, rec.m_buffer);
    }

    for (const HdatRecord& rec : m_hdatRecords) {
        if (!rec.m_blob.empty()) {
            const auto out = path / string2path(rec.m_filename + ".bin");
            if (skipExisting && std_fs::exists(out))
                continue;
            ByteArrayHolder buf;
            buf.ref() = rec.m_blob;
            writeFileFromHolderThrow(out, buf);
        }

        {
            const auto out = path / string2path(rec.m_filename + ".txt");
            if (skipExisting && std_fs::exists(out))
                continue;

            const auto chaptersStr = joinString(rec.m_txtChapters, std::string(g_hdatChapterSeparator));

            writeFileFromBufferThrow(out, chaptersStr);
        }
    }
}

void Archive::loadFromFolder(const std_path& path)
{
    m_isBinary = false;

    const auto jsonFilename = path / g_indexFileName;

    const std::string  buffer = readFileIntoBufferThrow(jsonFilename);
    const PropertyTree data   = readJsonFromBufferThrow(buffer);

    Reflection::PropertyTreeReader reader;
    reader.jsonToValue(data, *this);

    for (Record& rec : m_records) {
        const auto out = path / string2path(rec.m_filename);
        rec.m_buffer   = readFileIntoHolderThrow(out);
    }

    for (HdatRecord& rec : m_hdatRecords) {
        if (rec.m_hasBlob) {
            const auto      in  = path / string2path(rec.m_filename + ".bin");
            ByteArrayHolder buf = readFileIntoHolderThrow(in);
            rec.m_blob          = buf.ref();
        }

        {
            const auto        in          = path / string2path(rec.m_filename + ".txt");
            const std::string chaptersStr = readFileIntoBufferThrow(in);
            rec.m_txtChapters             = splitLine(chaptersStr, std::string(g_hdatChapterSeparator));
        }
    }
}

void Archive::convertToBinary()
{
    if (m_isBinary)
        throw std::runtime_error("Archive is already in binary format, no need for convertToBinary()");
    m_isBinary = true;

    if (m_format == BinaryFormat::HDAT)
        return;

    m_binaryRecords.clear();
    m_binaryRecordsUnnamed.clear();
    m_binaryRecordsSortedByOffset.clear();

    size_t order = 0;
    for (const Record& rec : m_records) {
        BinaryRecord brec;
        brec.m_filename        = rec.m_originalFilename;
        brec.m_filenameGarbage = rec.m_filenameGarbage;
        brec.m_unknown1        = rec.m_unknown1;
        brec.m_buffer          = rec.m_buffer;
        brec.m_size            = rec.m_buffer.size();
        brec.m_fullSize        = brec.m_size;
        if (rec.m_compressInArchive) {
            brec.m_fullSize       = rec.m_uncompressedSizeCache;
            brec.m_compressedSize = brec.m_size;
        }
        brec.m_binaryDataOrder = rec.m_binaryOrder;
        brec.m_headerOrder     = order++;

        if (rec.m_isPadding) {
            m_binaryRecordsUnnamed.push_back(std::move(brec));
        } else {
            m_binaryRecords.push_back(std::move(brec));
        }
    }

    auto updateIndex = [this]() {
        m_binaryRecordsSortedByOffset.clear();
        for (auto& rec : m_binaryRecords) {
            m_binaryRecordsSortedByOffset.push_back(&rec);
        }
        for (auto& rec : m_binaryRecordsUnnamed) {
            m_binaryRecordsSortedByOffset.push_back(&rec);
        }

        std::sort(m_binaryRecordsSortedByOffset.begin(), m_binaryRecordsSortedByOffset.end(), [](const BinaryRecord* rh, const BinaryRecord* lh) {
            return rh->m_binaryDataOrder < lh->m_binaryDataOrder;
        });
    };

    updateIndex();

    {
        const size_t baseOffset = 0
                                  + sizeof(g_lodSignature)
                                  + sizeof(m_lodFormat)
                                  + sizeof(uint32_t) // size
                                  + m_lodHeader.size()
                                  + 32 * m_binaryRecords.size();

        size_t offset = baseOffset;

        for (BinaryRecord* brec : m_binaryRecordsSortedByOffset) {
            brec->m_offset = offset;
            offset += brec->m_size;
        }
    }
}

void Archive::convertFromBinary()
{
    if (!m_isBinary)
        throw std::runtime_error("Archive is already in non-binary format, no need for convertToBinary()");
    m_isBinary = false;

    if (m_format == BinaryFormat::HDAT)
        return;

    m_records.clear();
    m_records.reserve(m_binaryRecords.size());
    for (BinaryRecord& brec : m_binaryRecords) {
        Record rec;
        rec.m_isPadding        = false;
        rec.m_buffer           = brec.m_buffer;
        rec.m_originalFilename = brec.m_filename;
        rec.m_filename         = rec.m_originalFilename;

        rec.m_unknown1        = brec.m_unknown1;
        rec.m_filenameGarbage = brec.m_filenameGarbage;
        rec.m_binaryOrder     = brec.m_binaryDataOrder;

        std::transform(rec.m_filename.begin(), rec.m_filename.end(), rec.m_filename.begin(), [](unsigned char c) { return std::tolower(c); });
        rec.m_compressInArchive     = brec.m_compressedSize > 0;
        rec.m_compressOnDisk        = rec.m_compressInArchive;
        rec.m_uncompressedSizeCache = rec.m_compressInArchive ? brec.m_fullSize : 0;

        if (rec.m_compressOnDisk)
            rec.m_filename = rec.m_filename + ".gz";
        m_records.push_back(std::move(rec));
    }
    for (BinaryRecord& brec : m_binaryRecordsUnnamed) {
        Record rec;
        rec.m_isPadding = true;
        rec.m_filename = rec.m_originalFilename = brec.m_filename;
        rec.m_binaryOrder                       = brec.m_binaryDataOrder;
        rec.m_buffer                            = brec.m_buffer;
        m_records.push_back(std::move(rec));
    }
}

void Archive::readBinaryLOD(ByteOrderDataStreamReader& stream)
{
    std::array<uint8_t, 4> signature;
    stream >> signature >> m_lodFormat;
    if (signature != g_lodSignature)
        throw std::runtime_error("LOD signature is not found");

    const uint32_t binaryRecordsCommonSize = stream.readSize();

    if (!binaryRecordsCommonSize)
        throw std::runtime_error("Archive contains 0 records, probably it's corrupted");

    m_lodHeader.resize(80);
    stream.readBlock(m_lodHeader.data(), m_lodHeader.size());

    m_binaryRecordsUnnamed.clear();
    m_binaryRecords.resize(binaryRecordsCommonSize);
    {
        size_t i = 0;
        for (auto& rec : m_binaryRecords) {
            stream >> rec;
            rec.m_headerOrder = i++;
        }
    }

    auto updateIndex = [this]() {
        m_binaryRecordsSortedByOffset.clear();
        for (auto& rec : m_binaryRecords) {
            m_binaryRecordsSortedByOffset.push_back(&rec);
        }
        for (auto& rec : m_binaryRecordsUnnamed) {
            m_binaryRecordsSortedByOffset.push_back(&rec);
        }

        std::sort(m_binaryRecordsSortedByOffset.begin(), m_binaryRecordsSortedByOffset.end(), [](const BinaryRecord* rh, const BinaryRecord* lh) {
            return rh->m_offset < lh->m_offset;
        });
    };

    updateIndex();

    const ptrdiff_t firstRecordOffset = static_cast<ptrdiff_t>(m_binaryRecordsSortedByOffset[0]->m_offset);

    {
        ptrdiff_t offsetExpected = firstRecordOffset, offsetPrev = firstRecordOffset;
        size_t    paddingBlobCounter = 0;
        for (auto* brec : m_binaryRecordsSortedByOffset) {
            if (offsetExpected != brec->m_offset) {
                size_t paddingSize = brec->m_offset - offsetExpected;

                ByteArrayHolder blob;
                blob.resize(paddingSize);
                BinaryRecord padbrec;
                padbrec.m_filename = "__PAD_" + std::to_string(paddingBlobCounter++);
                padbrec.m_size     = paddingSize;
                padbrec.m_fullSize = paddingSize;
                padbrec.m_buffer   = blob;
                padbrec.m_offset   = offsetExpected;

                std::cerr << "detected GAP in binary data at [" << brec->m_offset << "], creating padding blob '" << padbrec.m_filename << "' of size=" << paddingSize << '\n';

                m_binaryRecordsUnnamed.push_back(std::move(padbrec));
            }
            offsetPrev     = brec->m_offset;
            offsetExpected = offsetPrev + brec->m_size;

            brec->m_buffer.resize(brec->m_size);
        }
    }
    updateIndex();

    //std::cerr << "firstRecordOffset=" << firstRecordOffset << '\n';
    const auto currentOffset = stream.getBuffer().getOffsetRead();
    if (firstRecordOffset < currentOffset)
        throw std::runtime_error("First record offset [" + std::to_string(firstRecordOffset) + "] is less than current offset [" + std::to_string(currentOffset) + "]");
    if (firstRecordOffset > currentOffset) {
        size_t paddingSize = firstRecordOffset - currentOffset;

        ByteArrayHolder blob;
        blob.resize(paddingSize);
        BinaryRecord padbrec;
        padbrec.m_filename = "__PAD";
        padbrec.m_size     = paddingSize;
        padbrec.m_fullSize = paddingSize;
        padbrec.m_buffer   = blob;
        padbrec.m_offset   = currentOffset;

        std::cerr << "detected GAP in binary data at [" << currentOffset << "], creating padding blob '" << padbrec.m_filename << "' of size=" << paddingSize << '\n';

        m_binaryRecordsUnnamed.push_back(std::move(padbrec));
        updateIndex();
    }

    size_t i = 0;
    for (auto* brec : m_binaryRecordsSortedByOffset) {
        brec->m_binaryDataOrder = i++;
        const auto offset       = stream.getBuffer().getOffsetRead();
        if (0)
            std::cerr << "i= " << (i - 1) << " name=" << brec->m_filename << ", exp=" << brec->m_offset << ", s=" << brec->m_buffer.size() << ", current=" << offset << '\n';
        if (brec->m_offset == offset) {
            stream.readBlock(brec->m_buffer.data(), brec->m_buffer.size());
            continue;
        }

        throw std::runtime_error("Record offset mismatch for '" + brec->m_filename + "', expected=" + std::to_string(brec->m_offset) + ", but get=" + std::to_string(offset));
    }
}

void Archive::writeBinaryLOD(ByteOrderDataStreamWriter& stream) const
{
    stream << g_lodSignature << m_lodFormat;

    stream.writeSize(m_binaryRecords.size());

    stream.writeBlock(m_lodHeader.data(), m_lodHeader.size());

    for (auto& rec : m_binaryRecords) {
        stream << rec;
    }

    for (auto* brec : m_binaryRecordsSortedByOffset) {
        stream.writeBlock(brec->m_buffer.data(), brec->m_buffer.size());
    }
}

void Archive::readBinaryHDAT(ByteOrderDataStreamReader& stream)
{
    std::array<uint8_t, 4> signature;
    stream >> signature;
    if (signature != g_hdatSignature)
        throw std::runtime_error("HDAT signature is not found");

    uint32_t recordsCount;
    stream >> m_lodFormat >> recordsCount;

    m_hdatRecords.resize(recordsCount);
    for (HdatRecord& rec : m_hdatRecords) {
        stream >> rec.m_filename;
        stream >> rec.m_filenameAlt;

        stream >> rec.m_txtChapters;

        stream >> rec.m_hasBlob;
        if (rec.m_hasBlob) {
            stream >> rec.m_blob;
        }
        stream >> rec.m_params;
    }
}

void Archive::writeBinaryHDAT(ByteOrderDataStreamWriter& stream) const
{
    stream << g_hdatSignature;

    stream << m_lodFormat;
    stream.writeSize(m_hdatRecords.size());

    for (const HdatRecord& rec : m_hdatRecords) {
        stream << rec.m_filename;
        stream << rec.m_filenameAlt;

        stream << rec.m_txtChapters;

        stream << rec.m_hasBlob;
        if (rec.m_hasBlob) {
            stream << rec.m_blob;
        }
        stream << rec.m_params;
    }
}

void Archive::readBinarySND(ByteOrderDataStreamReader& stream)
{
}

void Archive::writeBinarySND(ByteOrderDataStreamWriter& stream) const
{
}

void Archive::readBinaryVID(ByteOrderDataStreamReader& stream)
{
}

void Archive::writeBinaryVID(ByteOrderDataStreamWriter& stream) const
{
}

void Archive::BinaryRecord::readBinary(ByteOrderDataStreamReader& stream)
{
    // ABCDEF.DAT0~~~00
    // we can have garbade bytes after filename. In this example,
    // 0..9 - string bytes.
    // 10 - null terminator.
    // 11,12,13 - non-null garbage
    // 14,15 - valid null padding.
    // we will save '~~~' as 3 garbage bytes after null nerminator.

    std::array<char, g_strSize + 1> strBuffer{};
    stream.readBlock(strBuffer.data(), g_strSize);
    m_filename = strBuffer.data();
    if (m_filename.size() < g_strSize - 1) {
        m_filenameGarbage.resize(g_strSize - m_filename.size() - 1);
        std::memcpy(m_filenameGarbage.data(), strBuffer.data() + m_filename.size() + 1, m_filenameGarbage.size());
        while (!m_filenameGarbage.empty() && (*m_filenameGarbage.rbegin() == 0))
            m_filenameGarbage.pop_back();
    }

    stream >> m_offset >> m_fullSize >> m_unknown1 >> m_compressedSize;
    m_size = m_compressedSize ? m_compressedSize : m_fullSize;
}

void Archive::BinaryRecord::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    if (m_filename.size() > g_strSize)
        throw std::runtime_error("Format support filenames with at most " + std::to_string(g_strSize) + " symbols: " + m_filename);

    std::array<char, g_strSize + 1> strBuffer{};
    std::memcpy(strBuffer.data(), m_filename.data(), m_filename.size());
    if (m_filenameGarbage.size())
        std::memcpy(strBuffer.data() + m_filename.size() + 1, m_filenameGarbage.data(), m_filenameGarbage.size());
    stream.writeBlock(strBuffer.data(), g_strSize);
    stream << m_offset << m_fullSize << m_unknown1 << m_compressedSize;
}

}
