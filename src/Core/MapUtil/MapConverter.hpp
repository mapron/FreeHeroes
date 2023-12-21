/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/FsUtils.hpp"
#include "MernelPlatform/Profiler.hpp"

#include "H3SVGMap.hpp"
#include "H3MMap.hpp"
#include "FHMap.hpp"

#include "MapUtilExport.hpp"

#include <iosfwd>

namespace FreeHeroes {
namespace Core {
class IGameDatabaseContainer;
class IRandomGeneratorFactory;
}

class MAPUTIL_EXPORT MapConverter {
public:
    struct BinaryPathsSet {
        Mernel::std_path m_binary;
        Mernel::std_path m_uncompressedBinary;
        Mernel::std_path m_json;
    };

    struct PathsSet {
        Mernel::std_path m_fhTemplate;
        Mernel::std_path m_fhMap;
        BinaryPathsSet   m_h3m;
        BinaryPathsSet   m_h3svg;
        Mernel::std_path m_jsonDiff;
    };

    struct Settings {
        PathsSet         m_inputs;
        PathsSet         m_outputs;
        bool             m_dumpUncompressedBuffers = false;
        bool             m_dumpBinaryDataJson      = false;
        bool             m_extraLogging            = false;
        uint64_t         m_seed                    = 0;
        Mernel::std_path m_rngUserSettings;
        std::string      m_stopAfterStage;
        std::string      m_showDebugStage;
        std::string      m_tileFilter;
        int              m_stopAfterHeat = 1000;
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

        CheckBinaryInputOutputEquality,
        CheckJsonInputOutputEquality,
        ConvertH3MToJson,
        ConvertJsonToH3M,
        ConvertH3SVGToJson,
        ConvertJsonToH3SVG,
        LoadFHTpl,
        LoadFH,
        SaveFH,
        LoadH3M,
        SaveH3M,
        FHMapToH3M,
        H3MToFHMap,
        FHTplToFHMap,
        H3MRoundTripJson,
        H3SVGRoundTripJson,
        H3MRoundTripFH,
    };

public:
    MapConverter(std::ostream&                        logOutput,
                 const Core::IGameDatabaseContainer*  databaseContainer,
                 const Core::IRandomGeneratorFactory* rngFactory,
                 Settings                             settings);

    void run(Task command, int recurse = 0) noexcept(false);

public: // todo:
    H3Map                   m_mapH3M;
    H3SVGMap                m_mapH3SVG;
    FHMap                   m_mapFH;
    PropertyTree            m_json;
    Mernel::ByteArrayHolder m_binaryBuffer;
    RawState                m_rawState          = RawState::Undefined;
    CompressionMethod       m_compressionMethod = CompressionMethod::Undefined;

private:
    using MemberProc = void (MapConverter::*)(void);
    void run(MemberProc member, const char* descr, int recurse) noexcept(false);

    // filenames
    void setInputFilename(const Mernel::std_path& path, std::string_view descr);
    void setOutputFilename(const Mernel::std_path& path, std::string_view descr);

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

    // Primitive tasks
    void binaryDeserializeH3M();
    void binarySerializeH3M();
    void propertySerializeH3M();
    void propertyDeserializeH3M();

    void binaryDeserializeH3SVG();
    void binarySerializeH3SVG();
    void propertySerializeH3SVG();
    void propertyDeserializeH3SVG();

    void propertySerializeFH();
    void propertyDeserializeFH();

    void convertFHtoH3M();
    void convertH3MtoFH();
    void convertFHTPLtoFH();

    void checkBinaryInputOutputEquality();
    void checkJsonInputOutputEquality();

private:
    void safeCopy(const Mernel::std_path& src, const Mernel::std_path& dest);

private:
    std::ostream&                              m_logOutput;
    const Core::IGameDatabaseContainer* const  m_databaseContainer;
    const Core::IRandomGeneratorFactory* const m_rngFactory;

    class ScopeLogger {
    public:
        ScopeLogger(std::string currentTask, int indent, std::ostream& output);
        ~ScopeLogger();

    private:
        std::string        m_currentTask;
        int                m_indent = 0;
        std::ostream&      m_output;
        Mernel::ScopeTimer m_timer;
    };

    Settings         m_settings;
    std::string      m_currentTask;
    std::string      m_currentIndent;
    Mernel::std_path m_inputFilename;
    Mernel::std_path m_outputFilename;
    std::set<size_t> m_ignoredOffsets;
};

MAPUTIL_EXPORT std::string taskToString(MapConverter::Task task);
MAPUTIL_EXPORT MapConverter::Task stringToTask(const std::string& str);

}
