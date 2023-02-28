/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapConverter.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"
#include "MernelPlatform/FileIOUtils.hpp"
#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/Compression.hpp"

#include "H3MConversion.hpp"

#include <iostream>

#define runMember(name) run(&MapConverter::name, #name, recurse + 1)
#define setInput(name) setInputFilename(m_settings.name, #name)
#define setOutput(name) setOutputFilename(m_settings.name, #name)

namespace Mernel::Reflection {
using namespace FreeHeroes;
ENUM_REFLECTION_STRINGIFY(
    MapConverter::Task,
    Invalid,
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
    H3MRoundTripFH)

}

namespace FreeHeroes {
using namespace Mernel;

class ConverterExpection : public std::runtime_error {
    using runtime_error::runtime_error;
};

std::string taskToString(MapConverter::Task task)
{
    auto str = Mernel::Reflection::EnumTraits::enumToString(task);
    return std::string(str.begin(), str.end());
}
MapConverter::Task stringToTask(const std::string& str)
{
    return Mernel::Reflection::EnumTraits::stringToEnum<MapConverter::Task>({ str.c_str(), str.size() });
}

MapConverter::MapConverter(std::ostream&                        logOutput,
                           const Core::IGameDatabaseContainer*  databaseContainer,
                           const Core::IRandomGeneratorFactory* rngFactory,
                           Settings                             settings)
    : m_logOutput(logOutput)
    , m_databaseContainer(databaseContainer)
    , m_rngFactory(rngFactory)
    , m_settings(std::move(settings))
{
}

void MapConverter::run(Task task, int recurse) noexcept(false)
{
    try {
        m_currentTask = taskToString(task);
        ScopeLogger scope(m_currentTask, recurse, m_logOutput);

        switch (task) {
            case Task::Invalid:
            {
                throw std::runtime_error("Can't execute invalid task.");
            }
            case Task::CheckBinaryInputOutputEquality:
            {
                checkBinaryInputOutputEquality();
            } break;
            case Task::CheckJsonInputOutputEquality:
            {
                checkJsonInputOutputEquality();
            } break;
            case Task::ConvertH3MToJson:
            {
                setInput(m_inputs.m_h3m.m_binary);
                runMember(readBinaryBufferData);

                runMember(detectCompression);
                runMember(uncompressRaw);
                if (m_settings.m_dumpUncompressedBuffers) {
                    setOutput(m_inputs.m_h3m.m_uncompressedBinary);
                    runMember(writeBinaryBufferDataAsUncompressed);
                }

                runMember(binaryDeserializeH3M);
                runMember(propertySerializeH3M);
                setOutput(m_outputs.m_h3m.m_json);
                runMember(writeJsonFromProperty);
            } break;
            case Task::ConvertJsonToH3M:
            {
                setInput(m_inputs.m_h3m.m_json);
                runMember(readJsonToProperty);

                runMember(propertyDeserializeH3M);
                runMember(binarySerializeH3M);
                if (m_settings.m_dumpUncompressedBuffers) {
                    setOutput(m_outputs.m_h3m.m_uncompressedBinary);
                    runMember(writeBinaryBufferDataAsUncompressed);
                }

                m_compressionMethod = CompressionMethod::Gzip;
                runMember(compressRaw);
                setOutput(m_outputs.m_h3m.m_binary);
                runMember(writeBinaryBufferData);
            } break;
            case Task::ConvertH3SVGToJson:
            {
                setInput(m_inputs.m_h3svg.m_binary);
                runMember(readBinaryBufferData);

                runMember(detectCompression);
                runMember(uncompressRaw);
                if (m_settings.m_dumpUncompressedBuffers) {
                    setOutput(m_inputs.m_h3svg.m_uncompressedBinary);
                    runMember(writeBinaryBufferDataAsUncompressed);
                }

                runMember(binaryDeserializeH3SVG);
                runMember(propertySerializeH3SVG);
                setOutput(m_outputs.m_h3svg.m_json);
                runMember(writeJsonFromProperty);
            } break;
            case Task::ConvertJsonToH3SVG:
            {
                setInput(m_inputs.m_h3svg.m_json);
                runMember(readJsonToProperty);

                runMember(propertyDeserializeH3SVG);
                runMember(binarySerializeH3SVG);
                if (m_settings.m_dumpUncompressedBuffers) {
                    setOutput(m_outputs.m_h3svg.m_uncompressedBinary);
                    runMember(writeBinaryBufferDataAsUncompressed);
                }

                m_compressionMethod = CompressionMethod::Gzip;
                runMember(compressRaw);
                setOutput(m_outputs.m_h3svg.m_binary);
                runMember(writeBinaryBufferData);
            } break;
            case Task::LoadFHTpl:
            {
                setInput(m_inputs.m_fhTemplate);
                runMember(readJsonToProperty);

                runMember(propertyDeserializeFH);
            } break;
            case Task::LoadFH:
            {
                setInput(m_inputs.m_fhMap);
                runMember(readJsonToProperty);

                runMember(propertyDeserializeFH);
            } break;
            case Task::SaveFH:
            {
                runMember(propertySerializeFH);
                setOutput(m_outputs.m_fhMap);
                runMember(writeJsonFromProperty);
            } break;
            case Task::LoadH3M:
            {
                setInput(m_inputs.m_h3m.m_binary);
                runMember(readBinaryBufferData);

                runMember(detectCompression);
                runMember(uncompressRaw);
                if (m_settings.m_dumpUncompressedBuffers) {
                    setOutput(m_inputs.m_h3m.m_uncompressedBinary);
                    runMember(writeBinaryBufferDataAsUncompressed);
                }

                runMember(binaryDeserializeH3M);
                if (m_settings.m_dumpBinaryDataJson) {
                    runMember(propertySerializeH3M);
                    setOutput(m_inputs.m_h3m.m_json);
                    runMember(writeJsonFromProperty);
                }
                runMember(convertH3MtoFH);
            } break;
            case Task::SaveH3M:
            {
                runMember(convertFHtoH3M);
                if (m_settings.m_dumpBinaryDataJson) {
                    runMember(propertySerializeH3M);
                    setOutput(m_outputs.m_h3m.m_json);
                    runMember(writeJsonFromProperty);
                }

                runMember(binarySerializeH3M);
                if (m_settings.m_dumpUncompressedBuffers) {
                    setOutput(m_outputs.m_h3m.m_uncompressedBinary);
                    runMember(writeBinaryBufferDataAsUncompressed);
                }

                m_compressionMethod = CompressionMethod::Gzip;
                runMember(compressRaw);
                setOutput(m_outputs.m_h3m.m_binary);
                runMember(writeBinaryBufferData);
            } break;
            case Task::FHMapToH3M:
            {
                run(Task::LoadFH, recurse + 1);
                run(Task::SaveH3M, recurse + 1);
            } break;
            case Task::H3MToFHMap:
            {
                run(Task::LoadH3M, recurse + 1);
                run(Task::SaveFH, recurse + 1);
            } break;
            case Task::FHTplToFHMap:
            {
                run(Task::LoadFHTpl, recurse + 1);

                runMember(convertFHTPLtoFH);

                run(Task::SaveFH, recurse + 1);
            } break;
            case Task::H3MRoundTripJson:
            {
                if (!m_settings.m_dumpUncompressedBuffers)
                    throw std::runtime_error("You need to set dumpUncompressedBuffers");

                run(Task::ConvertH3MToJson, recurse + 1);
                safeCopy(m_settings.m_outputs.m_h3m.m_json, m_settings.m_inputs.m_h3m.m_json);
                run(Task::ConvertJsonToH3M, recurse + 1);

                setInput(m_inputs.m_h3m.m_uncompressedBinary);
                setOutput(m_outputs.m_h3m.m_uncompressedBinary);
                run(Task::CheckBinaryInputOutputEquality, recurse + 1);
            } break;
            case Task::H3SVGRoundTripJson:
            {
                if (!m_settings.m_dumpUncompressedBuffers)
                    throw std::runtime_error("You need to set dumpUncompressedBuffers");

                run(Task::ConvertH3SVGToJson, recurse + 1);
                safeCopy(m_settings.m_outputs.m_h3svg.m_json, m_settings.m_inputs.m_h3svg.m_json);
                run(Task::ConvertJsonToH3SVG, recurse + 1);

                setInput(m_inputs.m_h3svg.m_uncompressedBinary);
                setOutput(m_outputs.m_h3svg.m_uncompressedBinary);
                run(Task::CheckBinaryInputOutputEquality, recurse + 1);
            } break;
            case Task::H3MRoundTripFH:
            {
                if (!m_settings.m_dumpUncompressedBuffers)
                    throw std::runtime_error("You need to set dumpUncompressedBuffers");

                run(Task::H3MToFHMap, recurse + 1);
                safeCopy(m_settings.m_outputs.m_fhMap, m_settings.m_inputs.m_fhMap);
                run(Task::FHMapToH3M, recurse + 1);

                setInput(m_inputs.m_h3m.m_json);
                setOutput(m_outputs.m_h3m.m_json);
                run(Task::CheckJsonInputOutputEquality, recurse + 1);

                setInput(m_inputs.m_h3m.m_uncompressedBinary);
                setOutput(m_outputs.m_h3m.m_uncompressedBinary);
                run(Task::CheckBinaryInputOutputEquality, recurse + 1);
            } break;
        }
    }
    catch (ConverterExpection&) {
        throw;
    }
    catch (std::exception& ex) {
        throw ConverterExpection("Exception thrown during execution of task '" + m_currentTask + "': " + std::string(ex.what()));
    }
}

void MapConverter::run(MemberProc member, const char* descr, int recurse) noexcept(false)
{
    m_currentIndent = "  ";
    for (int r = 0; r < recurse; ++r)
        m_currentIndent += "  ";
    m_currentTask = descr;
    ScopeLogger scope(descr, recurse, m_logOutput);
    (this->*member)();
}

void MapConverter::setInputFilename(const Mernel::std_path& path, std::string_view descr)
{
    if (path.empty())
        throw std::runtime_error("Path '" + std::string(descr) + "' is empty");

    if (!Mernel::std_fs::exists(path))
        throw std::runtime_error("Path '" + Mernel::path2string(path) + "' is not exist!");

    m_inputFilename = path;
}

void MapConverter::setOutputFilename(const Mernel::std_path& path, std::string_view descr)
{
    if (path.empty())
        throw std::runtime_error("Path '" + std::string(descr) + "' is empty");

    m_outputFilename = path;
}

void MapConverter::readBinaryBufferData()
{
    m_rawState = RawState::Undefined;

    m_binaryBuffer = Mernel::readFileIntoHolder(m_inputFilename);
    m_logOutput << m_currentIndent << "Read " << m_binaryBuffer.size() << " bytes from: " << Mernel::path2string(m_inputFilename) << '\n';

    m_rawState = RawState::Compressed;
}

void MapConverter::writeBinaryBufferData()
{
    if (m_rawState != RawState::Compressed)
        throw std::runtime_error("Buffer needs to be in Compressed state.");

    m_logOutput << m_currentIndent << "Write " << m_binaryBuffer.size() << " bytes to: " << Mernel::path2string(m_outputFilename) << '\n';
    Mernel::writeFileFromHolder(m_outputFilename, m_binaryBuffer);
}

void MapConverter::writeBinaryBufferDataAsUncompressed()
{
    if (m_rawState != RawState::Uncompressed)
        throw std::runtime_error("Buffer needs to be in Uncompressed state.");

    m_logOutput << m_currentIndent << "Write " << m_binaryBuffer.size() << " bytes to: " << Mernel::path2string(m_outputFilename) << '\n';
    Mernel::writeFileFromHolder(m_outputFilename, m_binaryBuffer);
}

void MapConverter::readJsonToProperty()
{
    m_logOutput << m_currentIndent << "Read: " << Mernel::path2string(m_inputFilename) << '\n';
    std::string buffer = Mernel::readFileIntoBuffer(m_inputFilename);
    m_json             = Mernel::readJsonFromBuffer(buffer);
}

void MapConverter::writeJsonFromProperty()
{
    m_logOutput << m_currentIndent << "Write: " << Mernel::path2string(m_outputFilename) << '\n';
    std::string buffer = Mernel::writeJsonToBuffer(m_json);
    Mernel::writeFileFromBuffer(m_outputFilename, buffer);
}

void MapConverter::detectCompression()
{
    m_compressionMethod = CompressionMethod::Undefined;
    if (m_rawState != RawState::Compressed)
        throw std::runtime_error("Buffer needs to be in Compressed state.");

    /// @todo: real detection of different methods!
    m_compressionMethod = CompressionMethod::Gzip;
}

void MapConverter::uncompressRaw()
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

    ByteArrayHolder out;
    Mernel::uncompressDataBuffer(m_binaryBuffer, out, { .m_type = Mernel::CompressionType::Gzip, .m_skipCRC = true }); // throws;
    m_binaryBuffer = std::move(out);

    m_rawState = RawState::Uncompressed;
}

void MapConverter::compressRaw()
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

    ByteArrayHolder out;
    Mernel::compressDataBuffer(m_binaryBuffer, out, { .m_type = Mernel::CompressionType::Gzip }); // throws;
    m_binaryBuffer = std::move(out);

    m_rawState = RawState::Compressed;
}

void MapConverter::binaryDeserializeH3M()
{
    if (m_rawState != RawState::Uncompressed)
        throw std::runtime_error("Buffer needs to be in Uncompressed state.");

    ByteOrderBuffer           bobuffer(m_binaryBuffer);
    ByteOrderDataStreamReader reader(bobuffer, ByteOrderDataStream::s_littleEndian);

    try {
        reader >> m_mapH3M;
        m_ignoredOffsets = m_mapH3M.m_ignoredOffsets;
    }
    catch (std::exception& ex) {
        throw std::runtime_error(ex.what() + std::string(", offset=") + std::to_string(bobuffer.getOffsetRead()));
    }
}

void MapConverter::binarySerializeH3M()
{
    m_binaryBuffer = {};
    ByteOrderBuffer           bobuffer(m_binaryBuffer);
    ByteOrderDataStreamWriter writer(bobuffer, ByteOrderDataStream::s_littleEndian);

    writer << m_mapH3M;

    m_rawState = RawState::Uncompressed;
}

void MapConverter::propertySerializeH3M()
{
    m_mapH3M.toJson(m_json);
}

void MapConverter::propertyDeserializeH3M()
{
    m_mapH3M.fromJson(m_json);
}

void MapConverter::binaryDeserializeH3SVG()
{
    if (m_rawState != RawState::Uncompressed)
        throw std::runtime_error("Buffer needs to be in Uncompressed state.");

    ByteOrderBuffer           bobuffer(m_binaryBuffer);
    ByteOrderDataStreamReader reader(bobuffer, ByteOrderDataStream::s_littleEndian);

    try {
        reader >> m_mapH3SVG;
    }
    catch (std::exception& ex) {
        throw std::runtime_error(ex.what() + std::string(", offset=") + std::to_string(bobuffer.getOffsetRead()));
    }
}

void MapConverter::binarySerializeH3SVG()
{
    m_binaryBuffer = {};
    ByteOrderBuffer           bobuffer(m_binaryBuffer);
    ByteOrderDataStreamWriter writer(bobuffer, ByteOrderDataStream::s_littleEndian);

    writer << m_mapH3SVG;

    m_rawState = RawState::Uncompressed;
}

void MapConverter::propertySerializeH3SVG()
{
    m_mapH3SVG.toJson(m_json);
}

void MapConverter::propertyDeserializeH3SVG()
{
    m_mapH3SVG.fromJson(m_json);
}

void MapConverter::propertySerializeFH()
{
    m_mapFH.toJson(m_json);
}

void MapConverter::propertyDeserializeFH()
{
    if (m_json["version"].getScalar().toString() == "HOTA")
        m_mapFH.m_version = Core::GameVersion::HOTA;
    else
        m_mapFH.m_version = Core::GameVersion::SOD;
    m_mapFH.fromJson(m_json, m_databaseContainer->getDatabase(m_mapFH.m_version));
}

void MapConverter::convertFHtoH3M()
{
    auto rng = m_rngFactory->create();
    rng->setSeed(m_mapFH.m_seed);

    auto* db = m_databaseContainer->getDatabase(m_mapFH.m_version);

    m_mapFH.initTiles(db);
    m_mapFH.m_tileMap.rngTiles(rng.get(), m_mapFH.m_template.m_roughTilePercentage);

    convertFH2H3M(m_mapFH, m_mapH3M, db);
}

void MapConverter::convertH3MtoFH()
{
    if (m_mapH3M.m_format >= MapFormat::HOTA1)
        m_mapFH.m_version = Core::GameVersion::HOTA;
    else
        m_mapFH.m_version = Core::GameVersion::SOD;
    convertH3M2FH(m_mapH3M, m_mapFH, m_databaseContainer->getDatabase(m_mapFH.m_version));
}

void MapConverter::convertFHTPLtoFH()
{
    auto rng = m_rngFactory->create();
    if (m_settings.m_seed)
        m_mapFH.m_seed = m_settings.m_seed;

    auto* db = m_databaseContainer->getDatabase(m_mapFH.m_version);

    if (!m_settings.m_rngUserSettings.empty()) {
        m_logOutput << m_currentIndent << "Read: " << Mernel::path2string(m_settings.m_rngUserSettings) << '\n';
        std::string buffer       = Mernel::readFileIntoBuffer(m_settings.m_rngUserSettings);
        auto        settingsJson = Mernel::readJsonFromBuffer(buffer);

        m_mapFH.applyRngUserSettings(settingsJson, db);
    }
    m_mapFH.rescaleToUserSize();

    rng->setSeed(m_mapFH.m_seed);

    generateFromTemplate(m_mapFH,
                         db,
                         rng.get(),
                         m_logOutput,
                         m_settings.m_stopAfterStage);
}

void MapConverter::checkBinaryInputOutputEquality()
{
    const std::string bufferIn  = Mernel::readFileIntoBuffer(m_inputFilename);
    const std::string bufferOut = Mernel::readFileIntoBuffer(m_outputFilename);

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
            if (m_ignoredOffsets.contains(i)) {
                skippedIgnoredOffsets++;
                continue;
            }
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

void MapConverter::checkJsonInputOutputEquality()
{
    auto jsonIn  = Mernel::readJsonFromBuffer(Mernel::readFileIntoBuffer(m_inputFilename));
    auto jsonOut = Mernel::readJsonFromBuffer(Mernel::readFileIntoBuffer(m_outputFilename));

    const bool result = jsonIn == jsonOut;
    m_logOutput << "Round-trip JSON result: " << (result ? "PASSED" : "FAILED") << '\n';
    if (result)
        return;

    {
        Mernel::writeFileFromBuffer(m_inputFilename, Mernel::writeJsonToBuffer(jsonIn, true));
        Mernel::writeFileFromBuffer(m_outputFilename, Mernel::writeJsonToBuffer(jsonOut, true));
    }

    const auto& jsonDiffIn  = m_settings.m_inputs.m_jsonDiff;
    const auto& jsonDiffOut = m_settings.m_outputs.m_jsonDiff;
    if (jsonDiffIn.empty() || jsonDiffOut.empty()) {
        m_logOutput << "Prodive jsonDiff fields to save difference to file.\n";
    } else {
        m_logOutput << "Saving diff to: in=" << jsonDiffIn << ", out=" << jsonDiffOut << '\n';
        PropertyTree::removeEqualValues(jsonIn, jsonOut);
        Mernel::writeFileFromBuffer(jsonDiffIn, Mernel::writeJsonToBuffer(jsonIn, true));
        Mernel::writeFileFromBuffer(jsonDiffOut, Mernel::writeJsonToBuffer(jsonOut, true));
    }

    throw std::runtime_error("Failed input == output");
}

void MapConverter::safeCopy(const Mernel::std_path& src, const Mernel::std_path& dest)
{
    if (src != dest) {
        Mernel::std_fs::remove(dest);
        Mernel::std_fs::copy_file(src, dest);
    }
}

MapConverter::ScopeLogger::ScopeLogger(std::string currentTask, int indent, std::ostream& output)
    : m_currentTask(std::move(currentTask))
    , m_indent(indent)
    , m_output(output)
{
    for (int r = 0; r < m_indent; ++r)
        m_output << "  ";
    m_output << "start " << m_currentTask << "\n";
}

MapConverter::ScopeLogger::~ScopeLogger()
{
    for (int r = 0; r < m_indent; ++r)
        m_output << "  ";
    m_output << "end " << m_currentTask << " (" << m_timer.elapsed() << " us.)\n";
}

}
