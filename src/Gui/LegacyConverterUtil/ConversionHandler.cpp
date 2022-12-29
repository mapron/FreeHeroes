/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ConversionHandler.hpp"

#include "Reflection/EnumTraitsMacro.hpp"

#include "IGameDatabase.hpp"
#include "FileIOUtils.hpp"
#include "Compression.hpp"

#include <iostream>

#define runMember(name) run(&ConversionHandler::name, #name, recurse + 1)
#define setInput(name) setInputFilename(m_settings.name, #name)
#define setOutput(name) setOutputFilename(m_settings.name, #name)

namespace FreeHeroes {

class ConverterExpection : public std::runtime_error {
    using runtime_error::runtime_error;
};

namespace Core::Reflection {

ENUM_REFLECTION_STRINGIY(ConversionHandler::Task,
                         Invalid,
                         Invalid,
                         UnpackDatToFolder,
                         PackFolderToDat,
                         ConvertDefToPng,
                         ConvertPngToDef,

                         DefRoundTripPng,
                         DatRoundTripFolder,
                         DatRoundTripMemory,
                         DatRoundTripMemoryWithConvert)

}

std::string taskToString(ConversionHandler::Task task)
{
    auto str = Core::Reflection::EnumTraits::enumToString(task);
    return std::string(str.begin(), str.end());
}
ConversionHandler::Task stringToTask(const std::string& str)
{
    return Core::Reflection::EnumTraits::stringToEnum<ConversionHandler::Task>({ str.c_str(), str.size() });
}

ConversionHandler::ConversionHandler(std::ostream&                       logOutput,
                                     const Core::IGameDatabaseContainer* databaseContainer,
                                     Settings                            settings)
    : m_logOutput(logOutput)
    , m_databaseContainer(databaseContainer)
    , m_settings(std::move(settings))
{
}

void ConversionHandler::run(Task task, int recurse) noexcept(false)
{
    try {
        m_currentTask = taskToString(task);
        ScopeLogger scope(m_currentTask, recurse, m_logOutput);

        switch (task) {
            case Task::Invalid:
            {
                throw std::runtime_error("Can't execute invalid task.");
            }
            case Task::UnpackDatToFolder:
            {
                setInput(m_inputs.m_datFile);
                runMember(readBinaryBufferData);

                runMember(binaryDeserializeArchive);
                runMember(convertArchiveFromBinary);

                setOutput(m_outputs.m_folder);
                runMember(writeArchiveToFolder);
            } break;
            case Task::PackFolderToDat:
            {
                setInput(m_inputs.m_folder);
                runMember(readArchiveFromFolder);

                runMember(convertArchiveToBinary);
                runMember(binarySerializeArchive);

                setOutput(m_outputs.m_datFile);
                runMember(writeBinaryBufferData);
            } break;
            case Task::ConvertDefToPng:
            {
                setInput(m_inputs.m_defFile);
                runMember(readBinaryBufferData);

                //

                setOutput(m_outputs.m_pngFile);
                runMember(writeBinaryBufferData);
            } break;
            case Task::ConvertPngToDef:
            {
                setInput(m_inputs.m_pngFile);
                runMember(readBinaryBufferData);

                //

                setOutput(m_outputs.m_defFile);
                runMember(writeBinaryBufferData);
            } break;
            case Task::DefRoundTripPng:
            {
                run(Task::ConvertDefToPng, recurse + 1);
                safeCopy(m_settings.m_outputs.m_pngFile, m_settings.m_inputs.m_pngFile);
                safeCopy(m_settings.m_outputs.m_pngJsonFile, m_settings.m_inputs.m_pngJsonFile);
                run(Task::ConvertPngToDef, recurse + 1);

                setInput(m_inputs.m_defFile);
                setOutput(m_outputs.m_defFile);
                runMember(checkBinaryInputOutputEquality);
            } break;
            case Task::DatRoundTripFolder:
            {
                run(Task::UnpackDatToFolder, recurse + 1);
                m_settings.m_inputs.m_folder = m_settings.m_outputs.m_folder;
                run(Task::PackFolderToDat, recurse + 1);

                setInput(m_inputs.m_datFile);
                setOutput(m_outputs.m_datFile);
                runMember(checkBinaryInputOutputEquality);
            } break;
            case Task::DatRoundTripMemory:
            case Task::DatRoundTripMemoryWithConvert:
            {
                setInput(m_inputs.m_datFile);
                runMember(readBinaryBufferData);

                runMember(binaryDeserializeArchive);
                if (task == Task::DatRoundTripMemoryWithConvert) {
                    runMember(convertArchiveFromBinary);
                    runMember(convertArchiveToBinary);
                }
                runMember(binarySerializeArchive);

                setOutput(m_outputs.m_datFile);
                runMember(writeBinaryBufferData);

                setInput(m_inputs.m_datFile);
                setOutput(m_outputs.m_datFile);
                runMember(checkBinaryInputOutputEquality);
            } break;
        }
        scope.markDone();
    }
    catch (ConverterExpection&) {
        throw;
    }
    catch (std::exception& ex) {
        throw ConverterExpection("Exception thrown during execution of task '" + m_currentTask + "': " + std::string(ex.what()));
    }
}

void ConversionHandler::run(MemberProc member, const char* descr, int recurse) noexcept(false)
{
    m_currentTask = descr;
    ScopeLogger scope(descr, recurse, m_logOutput);
    (this->*member)();
    scope.markDone();
}

void ConversionHandler::setInputFilename(const Core::std_path& path, std::string_view descr)
{
    if (path.empty())
        throw std::runtime_error("Path '" + std::string(descr) + "' is empty");

    if (!Core::std_fs::exists(path))
        throw std::runtime_error("Path '" + Core::path2string(path) + "' is not exist!");

    m_inputFilename = path;
}

void ConversionHandler::setOutputFilename(const Core::std_path& path, std::string_view descr)
{
    if (path.empty())
        throw std::runtime_error("Path '" + std::string(descr) + "' is empty");

    m_outputFilename = path;
}

void ConversionHandler::readBinaryBufferData()
{
    m_binaryBuffer = Core::readFileIntoHolderThrow(m_inputFilename);
}

void ConversionHandler::writeBinaryBufferData()
{
    Core::writeFileFromHolderThrow(m_outputFilename, m_binaryBuffer);
}

void ConversionHandler::binaryDeserializeArchive()
{
    ByteOrderBuffer           bobuffer(m_binaryBuffer);
    ByteOrderDataStreamReader reader(bobuffer, ByteOrderDataStream::createByteorderMask(ORDER_LE, ORDER_LE, ORDER_LE));

    try {
        m_archive = {};
        m_archive.detectFormat(m_inputFilename, reader);
        reader.getBuffer().setOffsetRead(0);
        reader >> m_archive;
        //m_ignoredOffsets = m_mapH3M.m_ignoredOffsets;
    }
    catch (std::exception& ex) {
        throw std::runtime_error(ex.what() + std::string(", offset=") + std::to_string(bobuffer.getOffsetRead()));
    }
}

void ConversionHandler::binarySerializeArchive()
{
    m_binaryBuffer = {};
    ByteOrderBuffer           bobuffer(m_binaryBuffer);
    ByteOrderDataStreamWriter writer(bobuffer, ByteOrderDataStream::createByteorderMask(ORDER_LE, ORDER_LE, ORDER_LE));

    writer << m_archive;
}

void ConversionHandler::convertArchiveToBinary()
{
    m_archive.convertToBinary();
}

void ConversionHandler::convertArchiveFromBinary()
{
    m_archive.convertFromBinary();
}

void ConversionHandler::writeArchiveToFolder()
{
    m_archive.saveToFolder(m_outputFilename, !m_settings.m_forceWrite);
}

void ConversionHandler::readArchiveFromFolder()
{
    m_archive = {};
    m_archive.loadFromFolder(m_inputFilename);
}

void ConversionHandler::checkBinaryInputOutputEquality()
{
    const std::string bufferIn  = Core::readFileIntoBufferThrow(m_inputFilename);
    const std::string bufferOut = Core::readFileIntoBufferThrow(m_outputFilename);

    m_logOutput << "(Input size=" << bufferIn.size() << ", Output size=" << bufferOut.size() << ")\n";

    auto ret = [this](bool status, bool condPassed = false) {
        if (status) {
            m_logOutput << "Round-trip binary result: " << (condPassed ? "CONDITIONALLY PASSED" : "PASSED") << "\n";
            return;
        }
        m_logOutput << "Round-trip binary result: FAILED\n";
        throw std::runtime_error("Failed input == output");
    };
    if (bufferIn == bufferOut)
        return ret(true);

    const bool sizeDifferent = bufferIn.size() != bufferOut.size();
    const auto minSize       = std::min(bufferIn.size(), bufferOut.size());
    const bool commonEqual   = (std::memcmp(bufferIn.data(), bufferOut.data(), minSize) == 0);

    if (commonEqual) {
        m_logOutput << "Common part is: EQUAL\n";
        assert(!sizeDifferent);
        return ret(false);
    }

    const int maxDiffCounter        = 10;
    int       skippedIgnoredOffsets = 0;
    int       diffOffsets           = 0;
    for (size_t i = 0; i < minSize; ++i) {
        if (bufferIn[i] != bufferOut[i]) {
            //            if (m_ignoredOffsets.contains(i)) {
            //                skippedIgnoredOffsets++;
            //                continue;
            //            }
            if (diffOffsets++ < maxDiffCounter)
                m_logOutput << "difference at [" << i << " / 0x" << std::hex << std::setfill('0') << i << "], in: 0x" << std::setw(2) << int(uint8_t(bufferIn[i]))
                            << ", out: 0x" << std::setw(2) << int(uint8_t(bufferOut[i])) << "\n"
                            << std::dec << std::setfill(' ');
        }
    }
    const bool condPassed = diffOffsets == 0 && !sizeDifferent;
    if (condPassed) {
        m_logOutput << "Common part is: CONDITIONALLY EQUAL (skippedIgnored:" << skippedIgnoredOffsets << ")\n";
        return ret(true, true);
    }
    m_logOutput << "Common part is: DIFFERENT (diff bytes: " << diffOffsets << ", skippedIgnored:" << skippedIgnoredOffsets << ")\n";

    return ret(false);
}

void ConversionHandler::safeCopy(const Core::std_path& src, const Core::std_path& dest)
{
    if (src != dest) {
        Core::std_fs::remove(dest);
        Core::std_fs::copy_file(src, dest);
    }
}

ConversionHandler::ScopeLogger::ScopeLogger(std::string currentTask, int indent, std::ostream& output)
    : m_currentTask(std::move(currentTask))
    , m_indent(indent)
    , m_output(output)
{
    for (int r = 0; r < m_indent; ++r)
        m_output << "  ";
    m_output << "start " << m_currentTask << "\n";
}

ConversionHandler::ScopeLogger::~ScopeLogger()
{
    for (int r = 0; r < m_indent; ++r)
        m_output << "  ";
    m_output << "end " << m_currentTask << " (" << m_timer.elapsed() << " us.)\n";
}

}
