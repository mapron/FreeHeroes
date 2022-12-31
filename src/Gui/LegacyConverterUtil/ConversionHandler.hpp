/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtils.hpp"
#include "Profiler.hpp"
#include "ByteBuffer.hpp"
#include "PropertyTree.hpp"

#include "LegacyConverterUtilExport.hpp"

#include <iosfwd>

namespace FreeHeroes {

class Archive;
class SpriteFile;

class LEGACYCONVERTERUTIL_EXPORT ConversionHandler {
public:
    struct BinaryPathsSet {
        Core::std_path m_binary;
        Core::std_path m_uncompressedBinary;
        Core::std_path m_json;
    };

    struct PathsSet {
        Core::std_path m_datFile;
        Core::std_path m_defFile;
        Core::std_path m_pngJsonFile;
        Core::std_path m_folder;
    };

    struct Settings {
        PathsSet m_inputs;
        PathsSet m_outputs;
        bool     m_forceWrite    = false;
        bool     m_cleanupFolder = false;
        bool     m_uncompress    = false;
        bool     m_prettyJson    = false;
    };

    enum class Task
    {
        Invalid,

        ArchiveLoadDat,
        ArchiveSaveDat,
        ArchiveLoadFolder,
        ArchiveSaveFolder,

        ArchiveRoundTripFolder,
        ArchiveRoundTripMemory,
        ArchiveRoundTripMemoryWithConvert,

        SpriteLoadDef,
        SpriteSaveDef,
        SpriteLoadFlat,
        SpriteSaveFlat,
        SpriteLoadPng,
        SpriteSavePng,

        SpriteRoundTripPng,
        SpriteRoundTripFlat,
    };

public:
    ConversionHandler(std::ostream& logOutput,
                      Settings      settings);
    ~ConversionHandler();

    void run(Task command, int recurse = 0) noexcept(false);

public:
    std::unique_ptr<Archive>    m_archive;
    std::unique_ptr<SpriteFile> m_sprite;
    ByteArrayHolder             m_binaryBuffer;
    PropertyTree                m_json;

private:
    using MemberProc = void (ConversionHandler::*)(void);
    void run(MemberProc member, const char* descr, int recurse) noexcept(false);

    // filenames
    void setInputFilename(const Core::std_path& path, std::string_view descr);
    void setOutputFilename(const Core::std_path& path, std::string_view descr);

    // raw I/O
    void readBinaryBufferData();
    void writeBinaryBufferData();

    // text I/O
    void readJsonToProperty();
    void writeJsonFromProperty();

    // Primitive tasks
    void binaryDeserializeArchive();
    void binarySerializeArchive();

    void convertArchiveToBinary();
    void convertArchiveFromBinary();

    void writeArchiveToFolder();
    void readArchiveFromFolder();

    void binaryDeserializeSprite();
    void binarySerializeSprite();

    void propertyDeserializeSprite();
    void propertySerializeSprite();

    void checkBinaryInputOutputEquality();

private:
    void safeCopy(const Core::std_path& src, const Core::std_path& dest);

private:
    std::ostream& m_logOutput;

    class ScopeLogger {
    public:
        ScopeLogger(std::string currentTask, int indent, std::ostream& output);
        ~ScopeLogger();
        void markDone() { m_done = true; }

    private:
        std::string   m_currentTask;
        int           m_indent = 0;
        std::ostream& m_output;
        bool          m_done = false;
        ScopeTimer    m_timer;
    };

    Settings       m_settings;
    std::string    m_currentTask;
    std::string    m_currentIndent;
    Core::std_path m_inputFilename;
    Core::std_path m_outputFilename;
};

LEGACYCONVERTERUTIL_EXPORT std::string taskToString(ConversionHandler::Task task);
LEGACYCONVERTERUTIL_EXPORT ConversionHandler::Task stringToTask(const std::string& str);

}
