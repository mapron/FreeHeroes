/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "Archive.hpp"

#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/FileIOUtils.hpp"
#include "MernelPlatform/StringUtils.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"
#include "MernelReflection/PropertyTreeWriter.hpp"

#include "ArchiveReflection.hpp"

#include "MernelPlatform/Compression.hpp"

#include <iostream>

namespace FreeHeroes {
using namespace Mernel;

namespace {
const std_path g_indexFileName = "archive_index.fh.json";

constexpr const std::array<uint8_t, 4> g_lodSignature{ { 0x4C, 0x4F, 0x44, 0x00 } };  // 'LOD\0'
constexpr const std::array<uint8_t, 4> g_hdatSignature{ { 0x48, 0x44, 0x41, 0x54 } }; // 'HDAT'
constexpr const size_t                 g_strSize    = 16;
constexpr const size_t                 g_strSndSize = 40;
constexpr const size_t                 g_strVidSize = 40;

constexpr const std::string_view g_hdatChapterSeparator{ "\r\n=============================\r\n" };
}

Archive::Archive(std::ostream* logOutput)
    : m_logOutput(logOutput ? logOutput : &std::cerr)
{
}

void Archive::clear()
{
    *this = Archive(m_logOutput);
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
        throw std::runtime_error("Failed to detect binary format for:" + path2string(path));
    }
}

void Archive::readBinary(ByteOrderDataStreamReader& stream)
{
    switch (m_format) {
        case BinaryFormat::LOD:
            break;
        case BinaryFormat::HDAT:
            readBinaryHDAT(stream);
            m_isBinary = true;
            return;
            break;
        case BinaryFormat::SND:
            break;
        case BinaryFormat::VID:
            break;
        default:
            throw std::runtime_error("Invalid binary format is set");
    }
    m_isBinary = true;

    if (m_format == BinaryFormat::LOD) {
        std::array<uint8_t, 4> signature;
        stream >> signature >> m_lodFormat;
        if (signature != g_lodSignature)
            throw std::runtime_error("LOD signature is not found");
    }

    const uint32_t binaryRecordsCommonSize = stream.readSize();

    if (!binaryRecordsCommonSize)
        throw std::runtime_error("Archive contains 0 records, probably it's corrupted");

    if (m_format == BinaryFormat::LOD) {
        m_lodHeader.resize(80);
        stream.readBlock(m_lodHeader.data(), m_lodHeader.size());
    }

    m_binaryRecordsUnnamed.clear();
    m_binaryRecords.resize(binaryRecordsCommonSize);
    {
        size_t i = 0;
        for (auto& rec : m_binaryRecords) {
            if (m_format == BinaryFormat::LOD) {
                rec.readBinaryLOD(stream);
            } else if (m_format == BinaryFormat::SND) {
                rec.readBinarySND(stream);
            } else if (m_format == BinaryFormat::VID) {
                rec.readBinaryVID(stream);
            }
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
    const ptrdiff_t dataEndOffset     = stream.getBuffer().getSize();

    {
        for (size_t i = 0; i < m_binaryRecordsSortedByOffset.size(); ++i) {
            const bool last     = i == m_binaryRecordsSortedByOffset.size() - 1;
            auto*      brec     = m_binaryRecordsSortedByOffset[i];
            auto*      brecNext = last ? nullptr : m_binaryRecordsSortedByOffset[i + 1];
            brec->m_offsetNext  = last ? dataEndOffset : brecNext->m_offset;
        }

        size_t paddingBlobCounter = 0;
        for (auto* brec : m_binaryRecordsSortedByOffset) {
            if (!brec->m_size)
                brec->m_size = brec->m_offsetNext - brec->m_offset;
            const ptrdiff_t possiblePadding = (brec->m_offsetNext - brec->m_offset) - brec->m_size;
            if (possiblePadding < 0) {
                *m_logOutput << "Data overlap detected, archive corrupted: offset=" << brec->m_offset
                             << ", size=" << brec->m_size << ", nextOffset=" << brec->m_offsetNext << '\n';
            }
            if (possiblePadding > 0) {
                const size_t paddingSize = possiblePadding;

                ByteArrayHolder blob;
                blob.resize(paddingSize);
                BinaryRecord padbrec;
                padbrec.m_basename = "__PAD_" + std::to_string(paddingBlobCounter++);
                padbrec.m_size     = paddingSize;
                padbrec.m_fullSize = paddingSize;
                padbrec.m_buffer   = blob;
                padbrec.m_offset   = brec->m_offsetNext - possiblePadding;

                *m_logOutput << "detected GAP in binary data at [" << padbrec.m_offset << ".." << brec->m_offsetNext << "], creating padding blob '" << padbrec.m_basename << "' of size=" << paddingSize << '\n';

                m_binaryRecordsUnnamed.push_back(std::move(padbrec));
            }

            brec->m_buffer.resize(brec->m_size);
        }
    }
    updateIndex();

    //*m_logOutput << "firstRecordOffset=" << firstRecordOffset << '\n';
    const auto currentOffset = stream.getBuffer().getOffsetRead();
    if (firstRecordOffset < currentOffset)
        throw std::runtime_error("First record offset [" + std::to_string(firstRecordOffset) + "] is less than current offset [" + std::to_string(currentOffset) + "]");
    if (firstRecordOffset > currentOffset) {
        size_t paddingSize = firstRecordOffset - currentOffset;

        ByteArrayHolder blob;
        blob.resize(paddingSize);
        BinaryRecord padbrec;
        padbrec.m_basename = "__PAD";
        padbrec.m_size     = paddingSize;
        padbrec.m_fullSize = paddingSize;
        padbrec.m_buffer   = blob;
        padbrec.m_offset   = currentOffset;

        *m_logOutput << "detected GAP in binary data at [" << currentOffset << ".." << firstRecordOffset << "], creating padding blob '" << padbrec.m_basename << "' of size=" << paddingSize << '\n';

        m_binaryRecordsUnnamed.push_back(std::move(padbrec));
        updateIndex();
    }

    size_t i = 0;
    for (auto* brec : m_binaryRecordsSortedByOffset) {
        brec->m_binaryDataOrder = i++;
        const auto offset       = stream.getBuffer().getOffsetRead();
        if (0)
            *m_logOutput << "i= " << (i - 1) << " name=" << brec->m_basename << ", exp=" << brec->m_offset << ", s=" << brec->m_buffer.size() << ", current=" << offset << '\n';
        if (brec->m_offset == offset) {
            stream.readBlock(brec->m_buffer.data(), brec->m_buffer.size());
            continue;
        }

        throw std::runtime_error("Record offset mismatch for '" + brec->m_basename + "', expected=" + std::to_string(brec->m_offset) + ", but get=" + std::to_string(offset));
    }
}

void Archive::writeBinary(ByteOrderDataStreamWriter& stream) const
{
    if (!m_isBinary)
        throw std::runtime_error("Need to convertToBinary() first!");

    switch (m_format) {
        case BinaryFormat::LOD:
            break;
        case BinaryFormat::HDAT:
            writeBinaryHDAT(stream);
            return;
            break;
        case BinaryFormat::SND:
            break;
        case BinaryFormat::VID:
            break;
        default:
            throw std::runtime_error("Invalid binary format is set");
    }
    if (m_format == BinaryFormat::LOD) {
        stream << g_lodSignature << m_lodFormat;
    }

    stream.writeSize(m_binaryRecords.size());

    if (m_format == BinaryFormat::LOD) {
        stream.writeBlock(m_lodHeader.data(), m_lodHeader.size());
    }

    for (auto& rec : m_binaryRecords) {
        if (m_format == BinaryFormat::LOD) {
            rec.writeBinaryLOD(stream);
        } else if (m_format == BinaryFormat::SND) {
            rec.writeBinarySND(stream);
        } else if (m_format == BinaryFormat::VID) {
            rec.writeBinaryVID(stream);
        }
    }

    for (auto* brec : m_binaryRecordsSortedByOffset) {
        stream.writeBlock(brec->m_buffer.data(), brec->m_buffer.size());
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

    std::string buffer = writeJsonToBuffer(data, true);
    writeFileFromBuffer(jsonFilename, buffer);

    for (const Record& rec : m_records) {
        const auto out = path / string2path(rec.fullname());
        if (skipExisting && std_fs::exists(out))
            continue;
        rec.m_bufferWithFile.readFromFile();

        rec.m_bufferWithFile.m_path = out;
        rec.m_bufferWithFile.writeToFile();
    }

    for (const HdatRecord& rec : m_hdatRecords) {
        if (rec.m_hasBlob) {
            const auto out = path / string2path(rec.m_basename + ".bin");
            if (skipExisting && std_fs::exists(out))
                continue;
            rec.m_bufferWithFile.readFromFile();

            rec.m_bufferWithFile.m_path = out;
            rec.m_bufferWithFile.writeToFile();
        }

        {
            const auto out = path / string2path(rec.m_basename + ".txt");
            if (skipExisting && std_fs::exists(out))
                continue;

            const auto chaptersStr = joinString(rec.m_txtChapters, std::string(g_hdatChapterSeparator));

            writeFileFromBuffer(out, chaptersStr);
        }
    }
}

void Archive::loadFromFolder(const std_path& path)
{
    m_isBinary = false;

    const auto jsonFilename = path / g_indexFileName;

    const std::string  buffer = readFileIntoBuffer(jsonFilename);
    const PropertyTree data   = readJsonFromBuffer(buffer);

    Reflection::PropertyTreeReader reader;
    reader.jsonToValue(data, *this);

    for (Record& rec : m_records) {
        const auto out              = path / string2path(rec.fullname());
        rec.m_bufferWithFile.m_path = out;
    }

    for (HdatRecord& rec : m_hdatRecords) {
        if (rec.m_hasBlob) {
            const auto in               = path / string2path(rec.m_basename + ".bin");
            rec.m_bufferWithFile.m_path = in;
        }

        {
            const auto        in          = path / string2path(rec.m_basename + ".txt");
            const std::string chaptersStr = readFileIntoBuffer(in);
            rec.m_txtChapters             = splitLine(chaptersStr, std::string(g_hdatChapterSeparator));
        }
    }
}

void Archive::createFromFolder(const Mernel::std_path& path, const std::vector<std::string>& extensions)
{
    m_format    = BinaryFormat::LOD;
    m_lodFormat = 200;
    m_lodHeader.resize(80);
    m_isBinary = false;

    for (auto&& it : Mernel::std_fs::directory_iterator(path)) {
        if (!it.is_regular_file())
            continue;
        auto   filename = it.path().filename();
        Record rec;
        rec.m_originalExtWithDot = Mernel::path2string(filename.extension());
        rec.m_extWithDot         = rec.m_originalExtWithDot;

        std::transform(rec.m_extWithDot.begin(), rec.m_extWithDot.end(), rec.m_extWithDot.begin(), [](unsigned char c) { return std::tolower(c); });
        if (std::find(extensions.cbegin(), extensions.cend(), rec.m_extWithDot) == extensions.cend())
            continue;

        rec.m_originalBasename = Mernel::path2string(filename.stem());
        rec.m_basename         = rec.m_originalBasename;
        std::transform(rec.m_basename.begin(), rec.m_basename.end(), rec.m_basename.begin(), [](unsigned char c) { return std::tolower(c); });

        rec.m_bufferWithFile.m_buffer   = readFileIntoHolder(it.path());
        rec.m_bufferWithFile.m_inMemory = true;
        m_records.push_back(std::move(rec));
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
        brec.m_basename = rec.m_originalBasename;
        brec.m_extNoDot = rec.m_originalExtWithDot;
        if (!brec.m_extNoDot.empty() && brec.m_extNoDot.starts_with('.'))
            brec.m_extNoDot.erase(0, 1);
        brec.m_filenameGarbage = rec.m_filenameGarbage;
        brec.m_unknown1        = rec.m_unknown1;
        brec.m_buffer          = rec.m_bufferWithFile.getBuffer();
        brec.m_size            = brec.m_buffer.size();
        brec.m_fullSize        = brec.m_size;
        if (rec.m_compressInArchive) {
            brec.m_fullSize       = rec.m_uncompressedSizeCache;
            brec.m_compressedSize = brec.m_size;
        }
        brec.m_binaryDataOrder = rec.m_binaryOrder;
        brec.m_headerOrder     = order++;

        if (!rec.m_compressOnDisk && rec.m_compressInArchive) {
            brec.m_fullSize = brec.m_buffer.size();
            ByteArrayHolder comp;
            compressDataBuffer(brec.m_buffer, comp, { .m_type = CompressionType::Zlib, .m_skipCRC = false });
            brec.m_buffer         = comp;
            brec.m_compressedSize = brec.m_size = comp.size();
        }
        if (rec.m_compressOnDisk && !rec.m_compressInArchive) {
            assert(0);
        }

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
        const size_t binaryRecordSize = m_format == BinaryFormat::LOD ? 32 : (m_format == BinaryFormat::SND ? 48 : 44);
        const size_t baseOffset       = 0
                                  + (m_format == BinaryFormat::LOD ? sizeof(g_lodSignature) : 0)
                                  + (m_format == BinaryFormat::LOD ? sizeof(m_lodFormat) : 0)
                                  + sizeof(uint32_t) // size
                                  + m_lodHeader.size()
                                  + binaryRecordSize * m_binaryRecords.size();

        size_t offset = baseOffset;

        for (BinaryRecord* brec : m_binaryRecordsSortedByOffset) {
            brec->m_offset = offset;
            offset += brec->m_size;
        }
    }
}

void Archive::convertFromBinary(bool uncompress)
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
        rec.m_isPadding                 = false;
        rec.m_bufferWithFile.m_buffer   = brec.m_buffer;
        rec.m_bufferWithFile.m_inMemory = true;

        rec.m_originalBasename   = brec.m_basename;
        rec.m_basename           = rec.m_originalBasename;
        rec.m_originalExtWithDot = brec.m_extNoDot;
        if (!rec.m_originalExtWithDot.empty())
            rec.m_originalExtWithDot.insert(0, ".");
        rec.m_extWithDot = rec.m_originalExtWithDot;
        std::transform(rec.m_basename.begin(), rec.m_basename.end(), rec.m_basename.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(rec.m_extWithDot.begin(), rec.m_extWithDot.end(), rec.m_extWithDot.begin(), [](unsigned char c) { return std::tolower(c); });

        rec.m_unknown1        = brec.m_unknown1;
        rec.m_filenameGarbage = brec.m_filenameGarbage;
        rec.m_binaryOrder     = brec.m_binaryDataOrder;

        rec.m_compressInArchive     = brec.m_compressedSize > 0;
        rec.m_compressOnDisk        = rec.m_compressInArchive && !uncompress;
        rec.m_uncompressedSizeCache = rec.m_compressInArchive ? brec.m_fullSize : 0;

        if (!rec.m_compressOnDisk && rec.m_compressInArchive) {
            ByteArrayHolder uncomp;
            uncompressDataBuffer(rec.m_bufferWithFile.m_buffer, uncomp, { .m_type = CompressionType::Zlib, .m_skipCRC = false });
            rec.m_bufferWithFile.m_buffer          = uncomp;
            [[maybe_unused]] const size_t unpacked = uncomp.size();
            assert(rec.m_uncompressedSizeCache == unpacked);
        }
        if (rec.m_compressOnDisk && !rec.m_compressInArchive) {
            assert(0);
        }

        if (rec.m_compressOnDisk)
            rec.m_extWithDot += ".gz";
        m_records.push_back(std::move(rec));
    }
    for (BinaryRecord& brec : m_binaryRecordsUnnamed) {
        Record rec;
        rec.m_isPadding = true;
        rec.m_basename = rec.m_originalBasename = brec.m_basename;
        rec.m_binaryOrder                       = brec.m_binaryDataOrder;
        rec.m_bufferWithFile.m_buffer           = brec.m_buffer;
        rec.m_bufferWithFile.m_inMemory         = true;
        m_records.push_back(std::move(rec));
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
        stream >> rec.m_basename;
        stream >> rec.m_filenameAlt;

        stream >> rec.m_txtChapters;

        stream >> rec.m_hasBlob;
        if (rec.m_hasBlob) {
            stream >> rec.m_bufferWithFile.m_buffer.ref();
            rec.m_bufferWithFile.m_inMemory = true;
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
        stream << rec.m_basename;
        stream << rec.m_filenameAlt;

        stream << rec.m_txtChapters;

        stream << rec.m_hasBlob;
        if (rec.m_hasBlob) {
            stream << rec.m_bufferWithFile.m_buffer.ref();
        }
        stream << rec.m_params;
    }
}

void Archive::BinaryRecord::readBinaryLOD(ByteOrderDataStreamReader& stream)
{
    // ABCDEF.DAT0~~~00
    // we can have garbade bytes after filename. In this example,
    // 0..9 - string bytes.
    // 10 - null terminator.
    // 11,12,13 - non-null garbage
    // 14,15 - valid null padding.
    // we will save '~~~' as 3 garbage bytes after null nerminator.
    std::string filename;
    stream.readStringWithGarbagePadding<g_strSize>(filename, m_filenameGarbage);
    auto parts = splitLine(filename, '.', true);
    m_basename = parts[0];
    if (parts.size() > 1)
        m_extNoDot = parts[1];

    stream >> m_offset >> m_fullSize >> m_unknown1 >> m_compressedSize;
    m_size = m_compressedSize ? m_compressedSize : m_fullSize;
}

void Archive::BinaryRecord::writeBinaryLOD(ByteOrderDataStreamWriter& stream) const
{
    std::string filename = m_basename;
    if (!m_extNoDot.empty())
        filename += '.' + m_extNoDot;
    stream.writeStringWithGarbagePadding<g_strSize>(filename, m_filenameGarbage);
    stream << m_offset << m_fullSize << m_unknown1 << m_compressedSize;
}

void Archive::BinaryRecord::readBinarySND(ByteOrderDataStreamReader& stream)
{
    stream.readStringWithGarbagePadding<g_strSndSize>(m_basename, m_filenameGarbage);

    stream >> m_offset >> m_fullSize;
    m_size     = m_fullSize;
    m_extNoDot = "wav";
}

void Archive::BinaryRecord::writeBinarySND(ByteOrderDataStreamWriter& stream) const
{
    std::vector<uint8_t> padding = m_filenameGarbage;
    stream.writeStringWithGarbagePadding<g_strSndSize>(m_basename, padding);
    stream << m_offset << m_fullSize;
}

void Archive::BinaryRecord::readBinaryVID(ByteOrderDataStreamReader& stream)
{
    std::string filename;
    stream.readStringWithGarbagePadding<g_strVidSize>(filename, m_filenameGarbage);
    auto parts = splitLine(filename, '.', true);
    m_basename = parts[0];
    if (parts.size() > 1)
        m_extNoDot = parts[1];

    stream >> m_offset;
}

void Archive::BinaryRecord::writeBinaryVID(ByteOrderDataStreamWriter& stream) const
{
    std::string filename = m_basename;
    if (!m_extNoDot.empty())
        filename += '.' + m_extNoDot;
    stream.writeStringWithGarbagePadding<g_strVidSize>(filename, m_filenameGarbage);
    stream << m_offset;
}

Mernel::ByteArrayHolder Archive::BufferWithFile::getBuffer()
{
    readFromFile();
    return m_buffer;
}

void Archive::BufferWithFile::readFromFile()
{
    if (m_inMemory)
        return;

    ByteArrayHolder buf = readFileIntoHolder(m_path);
    m_buffer            = buf;
    m_inMemory          = true;
}

void Archive::BufferWithFile::writeToFile()
{
    assert(m_inMemory);
    writeFileFromHolder(m_path, m_buffer);
}

}
