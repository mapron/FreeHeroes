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

#include "Archive.hpp"

#include "LegacyConverterUtilExport.hpp"

#include <iosfwd>

namespace FreeHeroes {
namespace Core {
class IGameDatabaseContainer;
}

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
        Core::std_path m_pngFile;
        Core::std_path m_pngJsonFile;
        Core::std_path m_folder;
    };

    struct Settings {
        PathsSet m_inputs;
        PathsSet m_outputs;
        bool     m_forceWrite    = false;
        bool     m_cleanupFolder = false;
    };

    enum class Task
    {
        Invalid,

        UnpackDatToFolder,
        PackFolderToDat,
        ConvertDefToPng,
        ConvertPngToDef,

        DefRoundTripPng,
        DatRoundTripFolder,
        DatRoundTripMemory,
        DatRoundTripMemoryWithConvert,
    };

public:
    ConversionHandler(std::ostream&                       logOutput,
                      const Core::IGameDatabaseContainer* databaseContainer,
                      Settings                            settings);

    void run(Task command, int recurse = 0) noexcept(false);

public:
    Archive         m_archive;
    ByteArrayHolder m_binaryBuffer;

private:
    using MemberProc = void (ConversionHandler::*)(void);
    void run(MemberProc member, const char* descr, int recurse) noexcept(false);

    // filenames
    void setInputFilename(const Core::std_path& path, std::string_view descr);
    void setOutputFilename(const Core::std_path& path, std::string_view descr);

    // raw I/O
    void readBinaryBufferData();
    void writeBinaryBufferData();

    // Primitive tasks
    void binaryDeserializeArchive();
    void binarySerializeArchive();

    void convertArchiveToBinary();
    void convertArchiveFromBinary();

    void writeArchiveToFolder();
    void readArchiveFromFolder();

    void checkBinaryInputOutputEquality();

private:
    void safeCopy(const Core::std_path& src, const Core::std_path& dest);

private:
    std::ostream&                             m_logOutput;
    const Core::IGameDatabaseContainer* const m_databaseContainer;

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
    Core::std_path m_inputFilename;
    Core::std_path m_outputFilename;
    bool           m_autoUncompress = false;
};

LEGACYCONVERTERUTIL_EXPORT std::string taskToString(ConversionHandler::Task task);
LEGACYCONVERTERUTIL_EXPORT ConversionHandler::Task stringToTask(const std::string& str);

}
