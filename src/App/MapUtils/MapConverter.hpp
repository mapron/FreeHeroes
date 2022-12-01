/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MapFormat.hpp"
#include "FsUtils.hpp"
#include "FHMap.hpp"

#include <iosfwd>

namespace FreeHeroes {
namespace Core {
class IGameDatabaseContainer;
class IRandomGeneratorFactory;
}

class MapConverter {
public:
    struct Settings {
        Core::std_path m_jsonInput;
        Core::std_path m_jsonOutput;
        Core::std_path m_h3mInput;
        Core::std_path m_h3mOutput;
        Core::std_path m_h3mUncompressedInput;
        Core::std_path m_h3mUncompressedOutput;
    };

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

    enum class Task
    {
        Invalid,
        // raw I/O
        ReadH3MRawData,
        WriteH3MRawData,
        DumpH3MRawDataAsUncompressedInput,
        DumpH3MRawDataAsUncompressedOutput,

        // text I/O
        ReadJsonToProperty,
        WriteJsonFromProperty,

        // Compression tasks
        DetectCompression,
        UncompressRaw,
        CompressRaw,

        // Primitive tasks
        ReadH3MRawToLegacyObject,
        WriteH3MRawFromLegacyObject,
        SerializeLegacyObjectToProperty,
        DeserializeLegacyObjectFromProperty,
        SerializeFHObjectToProperty,
        DeserializeFHObjectFromProperty,
        ConstructH3MFromFH,
        ConstructFHFromH3M,
        CheckH3MInputOutputEquality,

        // High-level tasks
        ConvertH3MToJson,
        ConvertJsonToH3M,
        FHMapToH3M,
        H3MToFHMap,
        H3MRoundTripJson,
        H3MRoundTripFH,
    };

public:
    MapConverter(std::ostream&                        logOutput,
                 const Core::IGameDatabaseContainer*  databaseContainer,
                 const Core::IRandomGeneratorFactory* rngFactory,
                 Settings                             settings);

    bool run(Task command, int recurse = 0) noexcept;

public: // todo:
    H3Map             m_mapLegacy;
    FHMap             m_mapFH;
    PropertyTree      m_json;
    ByteArrayHolder   m_h3mraw;
    RawState          m_rawState          = RawState::Undefined;
    CompressionMethod m_compressionMethod = CompressionMethod::Undefined;

private:
    std::ostream&                              m_logOutput;
    const Core::IGameDatabaseContainer* const  m_databaseContainer;
    const Core::IRandomGeneratorFactory* const m_rngFactory;

    Settings m_settings;
};

std::string        taskToString(MapConverter::Task task);
MapConverter::Task stringToTask(const std::string& str);

}
