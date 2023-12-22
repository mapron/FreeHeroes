/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/PropertyTree.hpp"
#include "MernelPlatform/ByteBuffer.hpp"

#include "MernelPlatform/FsUtils.hpp"
#include "MernelPlatform/Profiler.hpp"

namespace FreeHeroes {

struct MapConverterFile {
    enum class RawState
    {
        Undefined,
        Compressed,
        Uncompressed
    };

    enum class CompressionMethod
    {
        Undefined,
        NoCompression,
        Gzip,
    };

    Mernel::PropertyTree    m_json;
    Mernel::ByteArrayHolder m_binaryBuffer;
    RawState                m_rawState          = RawState::Undefined;
    CompressionMethod       m_compressionMethod = CompressionMethod::Undefined;
    Mernel::std_path        m_filename;

    // raw I/O
    void readBinaryBufferData();
    void writeBinaryBufferData();
    void writeBinaryBufferDataAsUncompressed();

    // text I/O
    void readJsonToProperty();
    void writeJsonFromProperty();

    // Compression tasks
    void detectCompression();
    void uncompressRaw();
    void compressRaw();
};

struct MapConverterFolder {
    Mernel::std_path              m_root;
    std::vector<MapConverterFile> m_files;

    void read();
    void write();
};

}
