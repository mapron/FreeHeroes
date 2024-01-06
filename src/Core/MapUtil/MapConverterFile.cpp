/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapConverterFile.hpp"

#include "MernelPlatform/FileIOUtils.hpp"
#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/FileFormatCSV.hpp"
#include "MernelPlatform/Compression.hpp"

namespace FreeHeroes {

namespace {
void compressGzip(Mernel::ByteArrayHolder& data)
{
    Mernel::ByteArrayHolder out;
    Mernel::compressDataBuffer(data, out, { .m_type = Mernel::CompressionType::Gzip }); // throws;
    data = std::move(out);
}
void uncompressGzip(Mernel::ByteArrayHolder& data)
{
    Mernel::ByteArrayHolder out;
    Mernel::uncompressDataBuffer(data, out, { .m_type = Mernel::CompressionType::Gzip, .m_skipCRC = true }); // throws;
    data = std::move(out);
}

}

void MapConverterFile::readBinaryBufferData()
{
    m_rawState = RawState::Undefined;

    m_binaryBuffer = Mernel::readFileIntoHolder(m_filename);
    m_rawState     = RawState::Compressed;
}

void MapConverterFile::writeBinaryBufferData()
{
    if (m_rawState != RawState::Compressed)
        throw std::runtime_error("Buffer needs to be in Compressed state.");

    Mernel::writeFileFromHolder(m_filename, m_binaryBuffer);
}

void MapConverterFile::writeBinaryBufferDataAsUncompressed()
{
    if (m_rawState != RawState::Uncompressed)
        throw std::runtime_error("Buffer needs to be in Uncompressed state.");

    Mernel::writeFileFromHolder(m_filename, m_binaryBuffer);
}

void MapConverterFile::readJsonToProperty()
{
    std::string buffer = Mernel::readFileIntoBuffer(m_filename);
    m_json             = Mernel::readJsonFromBuffer(buffer);
}

void MapConverterFile::writeJsonFromProperty()
{
    std::string buffer = Mernel::writeJsonToBuffer(m_json);
    Mernel::writeFileFromBuffer(m_filename, buffer);
}

void MapConverterFile::binaryBufferToString()
{
    m_bufferStr.resize(m_binaryBuffer.size());
    memcpy(m_bufferStr.data(), m_binaryBuffer.data(), m_binaryBuffer.size());
}

void MapConverterFile::binaryBufferFromString()
{
    m_binaryBuffer.resize(m_bufferStr.size());
    memcpy(m_binaryBuffer.data(), m_bufferStr.data(), m_bufferStr.size());
}

void MapConverterFile::readJsonToPropertyFromBuffer()
{
    binaryBufferToString();
    m_json = Mernel::readJsonFromBuffer(m_bufferStr);
}

void MapConverterFile::writeJsonFromPropertyToBuffer()
{
    m_bufferStr = Mernel::writeJsonToBuffer(m_json);
    binaryBufferFromString();
}

void MapConverterFile::readCsvFromBuffer()
{
    m_csv.useColumns = false;
    binaryBufferToString();
    if (!Mernel::readCSVFromBuffer(m_bufferStr, m_csv))
        throw std::runtime_error("Failed to parse csv");
}

void MapConverterFile::writeCsvToBuffer()
{
    m_csv.useColumns = false;
    m_csv.endsWithNL = true;
    if (!Mernel::writeCSVToBuffer(m_bufferStr, m_csv))
        throw std::runtime_error("Failed to write csv (HOW?)");
    binaryBufferFromString();
}

void MapConverterFile::detectCompression()
{
    m_compressionOffsets.clear();
    m_compressionMethod = CompressionMethod::Undefined;
    if (m_rawState != RawState::Compressed)
        throw std::runtime_error("Buffer needs to be in Compressed state.");

    if (m_binaryBuffer.size() < 10) {
        m_compressionMethod = CompressionMethod::NoCompression;
        return;
    }
    std::string buffer;
    buffer.resize(m_binaryBuffer.size());
    memcpy(buffer.data(), m_binaryBuffer.data(), m_binaryBuffer.size());

    /// @todo: other comressions?
    const std::string_view gzipDeflate("\x1f\x8b\x08", 3);
    if (buffer.starts_with(gzipDeflate)) {
        m_compressionMethod = CompressionMethod::Gzip;
        m_compressionOffsets.push_back(0);
        size_t nextPos = 1;
        while ((nextPos = buffer.find(gzipDeflate, nextPos)) != std::string::npos) {
            m_compressionOffsets.push_back(nextPos);
            nextPos += 1;
        }
        m_compressionOffsets.push_back(buffer.size());
    } else
        m_compressionMethod = CompressionMethod::NoCompression;
}

void MapConverterFile::uncompressRaw()
{
    if (m_rawState != RawState::Compressed)
        throw std::runtime_error("Buffer needs to be in Compressed state.");

    if (m_compressionMethod == CompressionMethod::Undefined)
        throw std::runtime_error("CompressionMethod need to be defined (or detected).");

    if (m_compressionMethod == CompressionMethod::NoCompression) {
        m_rawState = RawState::Uncompressed;
        return;
    }
    if (m_compressionMethod != CompressionMethod::Gzip) {
        assert("Invalid compression method");
        return;
    }

    uncompressGzip(m_binaryBuffer);

    m_rawState = RawState::Uncompressed;
}

void MapConverterFile::compressRaw()
{
    if (m_rawState != RawState::Uncompressed)
        throw std::runtime_error("Buffer needs to be in Uncompressed state.");

    if (m_compressionMethod == CompressionMethod::Undefined)
        throw std::runtime_error("CompressionMethod need to be defined (or detected).");

    if (m_compressionMethod == CompressionMethod::NoCompression) {
        m_rawState = RawState::Compressed;
        return;
    }
    if (m_compressionMethod != CompressionMethod::Gzip) {
        assert("Invalid compression method");
        return;
    }

    compressGzip(m_binaryBuffer);

    m_rawState = RawState::Compressed;
}

void MapConverterFile::splitCompressedDataByOffsets()
{
    if (m_compressionOffsets.empty())
        throw std::runtime_error("No compress offsets were detected.");

    m_binaryParts.clear();
    for (size_t i = 0; i < m_compressionOffsets.size() - 1; i++) {
        const size_t start = m_compressionOffsets[i];
        const size_t end   = m_compressionOffsets[i + 1];
        assert(start < end);
        const size_t            size = end - start;
        Mernel::ByteArrayHolder part;
        part.resize(size);
        memcpy(part.data(), m_binaryBuffer.data() + start, size);
        m_binaryParts.push_back(std::move(part));
    }
}

void MapConverterFile::uncompressRawParts()
{
    if (m_rawState != RawState::Compressed)
        throw std::runtime_error("Buffer needs to be in Compressed state.");

    if (m_compressionMethod == CompressionMethod::Undefined)
        throw std::runtime_error("CompressionMethod need to be defined (or detected).");

    if (m_compressionMethod != CompressionMethod::Gzip) {
        assert("Invalid compression method");
    }
    for (auto& part : m_binaryParts) {
        uncompressGzip(part);
    }
    m_rawState = RawState::Uncompressed;
}

void MapConverterFile::compressRawParts()
{
    if (m_rawState != RawState::Uncompressed)
        throw std::runtime_error("Buffer needs to be in Uncompressed state.");

    if (m_compressionMethod == CompressionMethod::Undefined)
        throw std::runtime_error("CompressionMethod need to be defined (or detected).");

    if (m_compressionMethod == CompressionMethod::NoCompression) {
        m_rawState = RawState::Compressed;
        return;
    }
    if (m_compressionMethod != CompressionMethod::Gzip) {
        assert("Invalid compression method");
        return;
    }

    for (auto& part : m_binaryParts) {
        compressGzip(part);
    }
    m_rawState = RawState::Compressed;
}

void MapConverterFolder::read()
{
    throw std::runtime_error("unsupported yet");
}

void MapConverterFolder::write()
{
    if (!Mernel::std_fs::exists(m_root))
        Mernel::std_fs::create_directories(m_root);

    for (auto& file : m_files) {
        file.m_filename = m_root / file.m_filename.filename();
        file.writeBinaryBufferDataAsUncompressed();
    }
}

}
