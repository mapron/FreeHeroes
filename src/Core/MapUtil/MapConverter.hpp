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
#include "H3CCampaign.hpp"
#include "FHMap.hpp"
#include "H3Template.hpp"

#include "MapConverterFile.hpp"

#include "MapUtilExport.hpp"

#include <iosfwd>

namespace FreeHeroes {
namespace Core {
class IGameDatabaseContainer;
class IRandomGeneratorFactory;
}

class MAPUTIL_EXPORT MapConverter {
public:
    using RawState          = MapConverterFile::RawState;
    using CompressionMethod = MapConverterFile::CompressionMethod;

    class MAPUTIL_EXPORT RoundTripException : public ::std::runtime_error {
        using ::std::runtime_error::runtime_error;
    };

    struct BinaryPathsSet {
        Mernel::std_path m_binary;
        Mernel::std_path m_uncompressedBinary;
        Mernel::std_path m_json;
    };

    struct PathsSet {
        Mernel::std_path m_fhTemplate;
        Mernel::std_path m_fhMap;
        BinaryPathsSet   m_h3c;
        BinaryPathsSet   m_h3m;
        BinaryPathsSet   m_h3svg;
        BinaryPathsSet   m_h3tpl;
        Mernel::std_path m_jsonDiff;
        Mernel::std_path m_folder;
    };

    struct Settings {
        PathsSet m_inputs;
        PathsSet m_outputs;
        bool     m_dumpUncompressedBuffers = false;
        bool     m_dumpBinaryDataJson      = false;
    };

    struct TemplateSettings {
        bool             m_extraLogging = false;
        uint64_t         m_seed         = 0;
        Mernel::std_path m_rngUserSettings;
        std::string      m_stopAfterStage;
        std::string      m_showDebugStage;
        std::string      m_tileFilter;
        int              m_stopAfterHeat = 1000;
    };

    enum class Task
    {
        Invalid,

        // low-level
        LoadH3MRaw,
        SaveH3MRaw,
        LoadH3M,
        SaveH3M,
        ConvertH3MToJson,
        ConvertJsonToH3M,
        H3MRoundTripJson,
        H3MRoundTripFH,

        LoadH3SVGRaw,
        SaveH3SVGRaw,
        LoadH3SVG,
        SaveH3SVG,
        ConvertH3SVGToJson,
        ConvertJsonToH3SVG,
        H3SVGRoundTripJson,
        H3SVGRoundTripFH,

        LoadH3C,
        SaveH3C,
        ConvertH3CToFolderList,
        ConvertFolderListToH3C,

        LoadH3TPLRaw,
        SaveH3TPLRaw,
        LoadH3TPL,
        SaveH3TPL,
        ConvertH3TPLToJson,
        ConvertJsonToH3TPL,
        H3TPLRoundTripJson,
        H3TPLRoundTripFH,

        // high-level
        LoadFHTpl,
        SaveFHTpl,
        LoadFH,
        SaveFH,
        LoadFolder,
        SaveFolder,

        // convenience calls
        FHMapToH3M,
        H3MToFHMap,
        FHMapToH3SVG,
        H3SVGToFHMap,
        FHTplToH3TPL,
        H3TPLToFHTpl,
        H3CToFolder,

        GenerateFHMap,

        // utilities
        CheckBinaryInputOutputEquality,
        CheckJsonInputOutputEquality,
    };

public:
    MapConverter(std::ostream&                        logOutput,
                 const Core::IGameDatabaseContainer*  databaseContainer,
                 const Core::IRandomGeneratorFactory* rngFactory,
                 Settings                             settings);

    void setSettings(Settings settings) { m_settings = std::move(settings); }
    void setTemplateSettings(TemplateSettings settings) { m_templateSettings = std::move(settings); }

    void run(Task command, int recurse = 0) noexcept(false);

public: // todo:
    H3Map              m_mapH3M;
    H3SVGMap           m_mapH3SVG;
    H3CCampaign        m_mapH3C;
    FHMap              m_mapFH;
    H3Template         m_templateH3;
    MapConverterFile   m_mainFile;
    MapConverterFolder m_folder;

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
    void uncompressRawParts();
    void compressRawParts();

    // Primitive tasks
    void binaryDeserializeH3M();
    void binarySerializeH3M();
    void propertySerializeH3M();
    void propertyDeserializeH3M();

    void binaryDeserializeH3SVG();
    void binarySerializeH3SVG();
    void propertySerializeH3SVG();
    void propertyDeserializeH3SVG();

    void binaryDeserializeH3C();
    void binarySerializeH3C();
    void propertySerializeH3C();
    void propertyDeserializeH3C();

    void binaryDeserializeH3TPL();
    void binarySerializeH3TPL();
    void propertySerializeH3TPL();
    void propertyDeserializeH3TPL();

    void propertySerializeFH();
    void propertyDeserializeFH();
    void propertySerializeFHTpl();
    void propertyDeserializeFHTpl();

    void convertFHtoH3M();
    void convertH3MtoFH();
    void convertFHtoH3SVG();
    void convertH3SVGtoFH();
    void convertFHTpltoH3TPL();
    void convertH3TPLtoFHTpl();

    void generateFHMapFromFHTpl();

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
    TemplateSettings m_templateSettings;
    std::string      m_currentTask;
    std::string      m_currentIndent;
    Mernel::std_path m_inputFilename;
    Mernel::std_path m_outputFilename;
    std::set<size_t> m_ignoredOffsets;
};

MAPUTIL_EXPORT std::string taskToString(MapConverter::Task task);
MAPUTIL_EXPORT MapConverter::Task stringToTask(const std::string& str);

}
