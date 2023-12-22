/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapConverterFile.hpp"

#include "MernelPlatform/FileIOUtils.hpp"
#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/Compression.hpp"

namespace FreeHeroes {

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

void MapConverterFile::detectCompression()
{
    m_compressionMethod = CompressionMethod::Undefined;
    if (m_rawState != RawState::Compressed)
        throw std::runtime_error("Buffer needs to be in Compressed state.");

    std::string buffer = Mernel::readFileIntoBuffer(m_filename);
    if (buffer.size() < 10) {
        m_compressionMethod = CompressionMethod::NoCompression;
        return;
    }

    /// @todo: other comressions? look further in file, if compresion is not on 0 byte?
    // gzip = 1f 8b
    if (buffer.starts_with("\x1f\x8b"))
        m_compressionMethod = CompressionMethod::Gzip;
    else
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

    Mernel::ByteArrayHolder out;
    Mernel::uncompressDataBuffer(m_binaryBuffer, out, { .m_type = Mernel::CompressionType::Gzip, .m_skipCRC = true }); // throws;
    m_binaryBuffer = std::move(out);

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

    Mernel::ByteArrayHolder out;
    Mernel::compressDataBuffer(m_binaryBuffer, out, { .m_type = Mernel::CompressionType::Gzip }); // throws;
    m_binaryBuffer = std::move(out);

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
