/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapConverter.hpp"

#include "MernelReflection/EnumTraitsMacro.hpp"

#include "MernelPlatform/FileIOUtils.hpp"
#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/FileFormatCSV.hpp"

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"
#include "FHTemplateProcessor.hpp"

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
    CheckJsonInputOutputEquality)

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
        using enum Task;
        switch (task) {
            case Task::Invalid:
            {
                throw std::runtime_error("Can't execute invalid task.");
            }
                // low-level
                // -----------------H3M---------------------

            case Task::LoadH3MRaw:
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
            } break;
            case Task::SaveH3MRaw:
            {
                runMember(binarySerializeH3M);
                if (m_settings.m_dumpUncompressedBuffers) {
                    setOutput(m_outputs.m_h3m.m_uncompressedBinary);
                    runMember(writeBinaryBufferDataAsUncompressed);
                }

                m_mainFile.m_compressionMethod = CompressionMethod::Gzip;
                runMember(compressRaw);
                setOutput(m_outputs.m_h3m.m_binary);
                runMember(writeBinaryBufferData);
            } break;
            case Task::LoadH3M:
            {
                run(Task::LoadH3MRaw, recurse + 1);
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
                run(Task::SaveH3MRaw, recurse + 1);
            } break;
            case Task::ConvertH3MToJson:
            {
                run(Task::LoadH3MRaw, recurse + 1);

                runMember(propertySerializeH3M);
                setOutput(m_outputs.m_h3m.m_json);
                runMember(writeJsonFromProperty);
            } break;
            case Task::ConvertJsonToH3M:
            {
                setInput(m_inputs.m_h3m.m_json);
                runMember(readJsonToProperty);

                runMember(propertyDeserializeH3M);

                run(Task::SaveH3MRaw, recurse + 1);
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

                // ----------------H3SVG---------------------

            case Task::LoadH3SVGRaw:
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
            } break;
            case Task::SaveH3SVGRaw:
            {
                runMember(binarySerializeH3SVG);
                if (m_settings.m_dumpUncompressedBuffers) {
                    setOutput(m_outputs.m_h3svg.m_uncompressedBinary);
                    runMember(writeBinaryBufferDataAsUncompressed);
                }

                m_mainFile.m_compressionMethod = CompressionMethod::Gzip;
                runMember(compressRaw);
                setOutput(m_outputs.m_h3svg.m_binary);
                runMember(writeBinaryBufferData);
            } break;
            case Task::LoadH3SVG:
            {
                run(Task::LoadH3SVGRaw, recurse + 1);
                if (m_settings.m_dumpBinaryDataJson) {
                    runMember(propertySerializeH3SVG);
                    setOutput(m_inputs.m_h3svg.m_json);
                    runMember(writeJsonFromProperty);
                }
                runMember(convertH3SVGtoFH);
            } break;
            case Task::SaveH3SVG:
            {
                runMember(convertFHtoH3SVG);
                if (m_settings.m_dumpBinaryDataJson) {
                    runMember(propertySerializeH3SVG);
                    setOutput(m_outputs.m_h3svg.m_json);
                    runMember(writeJsonFromProperty);
                }
                run(Task::SaveH3SVGRaw, recurse + 1);
            } break;
            case Task::ConvertH3SVGToJson:
            {
                run(Task::LoadH3SVGRaw, recurse + 1);

                runMember(propertySerializeH3SVG);
                setOutput(m_outputs.m_h3svg.m_json);
                runMember(writeJsonFromProperty);
            } break;
            case Task::ConvertJsonToH3SVG:
            {
                setInput(m_inputs.m_h3svg.m_json);
                runMember(readJsonToProperty);

                runMember(propertyDeserializeH3SVG);

                run(Task::SaveH3SVGRaw, recurse + 1);
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
            case Task::H3SVGRoundTripFH:
            {
                if (!m_settings.m_dumpUncompressedBuffers)
                    throw std::runtime_error("You need to set dumpUncompressedBuffers");

                run(Task::H3SVGToFHMap, recurse + 1);
                safeCopy(m_settings.m_outputs.m_fhMap, m_settings.m_inputs.m_fhMap);
                run(Task::FHMapToH3SVG, recurse + 1);

                setInput(m_inputs.m_h3svg.m_json);
                setOutput(m_outputs.m_h3svg.m_json);
                run(Task::CheckJsonInputOutputEquality, recurse + 1);

                setInput(m_inputs.m_h3svg.m_uncompressedBinary);
                setOutput(m_outputs.m_h3svg.m_uncompressedBinary);
                run(Task::CheckBinaryInputOutputEquality, recurse + 1);
            } break;

                // ----------------H3C---------------------

            case Task::LoadH3C:
            {
                setInput(m_inputs.m_h3c.m_binary);
                runMember(readBinaryBufferData);

                runMember(detectCompression);
                runMember(uncompressRawParts);
                if (m_settings.m_dumpUncompressedBuffers) {
                    //setOutput(m_inputs.m_h3c.m_uncompressedBinary);
                    //runMember(writeBinaryBufferDataAsUncompressed);
                }

                runMember(binaryDeserializeH3C);
                if (m_settings.m_dumpBinaryDataJson) {
                    runMember(propertySerializeH3C);
                    setOutput(m_inputs.m_h3c.m_json);
                    runMember(writeJsonFromProperty);
                }
                //runMember(convertH3CtoFH);
            } break;
            case Task::SaveH3C:
            {
                //runMember(convertFHtoH3C);
                if (m_settings.m_dumpBinaryDataJson) {
                    runMember(propertySerializeH3C);
                    setOutput(m_outputs.m_h3c.m_json);
                    runMember(writeJsonFromProperty);
                }

                runMember(binarySerializeH3C);
                if (m_settings.m_dumpUncompressedBuffers) {
                    setOutput(m_outputs.m_h3c.m_uncompressedBinary);
                    runMember(writeBinaryBufferDataAsUncompressed);
                }

                m_mainFile.m_compressionMethod = CompressionMethod::Gzip;
                runMember(compressRaw);
                setOutput(m_outputs.m_h3c.m_binary);
                runMember(writeBinaryBufferData);
            } break;

            case Task::ConvertH3CToFolderList:
            {
                m_folder.m_files.clear();
                for (auto& sc : m_mapH3C.m_scenarios)
                    m_folder.m_files.push_back(MapConverterFile{
                        .m_binaryBuffer = sc.m_data,
                        .m_rawState     = RawState::Uncompressed,
                        .m_filename     = sc.m_filename,
                    });
                MapConverterFile meta;
                m_mapH3C.toJson(meta.m_json);
                meta.m_filename = "meta.json";
                meta.writeJsonFromPropertyToBuffer();
                meta.m_rawState = RawState::Uncompressed;
                m_folder.m_files.push_back(std::move(meta));

            } break;
            case ConvertFolderListToH3C:
            {
                throw std::runtime_error("Unsupported yet");
            } break;

                // ----------------H3TPL----------------------

            case Task::LoadH3TPLRaw:
            {
                setInput(m_inputs.m_h3tpl.m_binary);
                runMember(readBinaryBufferData);

                m_mainFile.m_compressionMethod = CompressionMethod::NoCompression;
                m_mainFile.m_rawState          = RawState::Uncompressed;

                m_mainFile.readCsvFromBuffer();
                m_mainFile.writeCsvToBuffer();
                if (m_settings.m_dumpUncompressedBuffers) {
                    setOutput(m_inputs.m_h3tpl.m_uncompressedBinary);
                    runMember(writeBinaryBufferDataAsUncompressed);
                }
                runMember(binaryDeserializeH3TPL);
            } break;
            case Task::SaveH3TPLRaw:
            {
                runMember(binarySerializeH3TPL);

                m_mainFile.writeCsvToBuffer();

                if (m_settings.m_dumpUncompressedBuffers) {
                    setOutput(m_outputs.m_h3tpl.m_uncompressedBinary);
                    runMember(writeBinaryBufferDataAsUncompressed);
                }

                m_mainFile.m_compressionMethod = CompressionMethod::NoCompression;
                m_mainFile.m_rawState          = RawState::Compressed;

                setOutput(m_outputs.m_h3tpl.m_binary);
                runMember(writeBinaryBufferData);
            } break;
            case Task::LoadH3TPL:
            {
                run(Task::LoadH3TPLRaw, recurse + 1);
                if (m_settings.m_dumpBinaryDataJson) {
                    runMember(propertySerializeH3TPL);
                    setOutput(m_inputs.m_h3tpl.m_json);
                    runMember(writeJsonFromProperty);
                }
                runMember(convertH3TPLtoFHTpl);
            } break;
            case Task::SaveH3TPL:
            {
                runMember(convertFHTpltoH3TPL);
                if (m_settings.m_dumpBinaryDataJson) {
                    runMember(propertySerializeH3TPL);
                    setOutput(m_outputs.m_h3tpl.m_json);
                    runMember(writeJsonFromProperty);
                }
                run(Task::SaveH3TPLRaw, recurse + 1);
            } break;
            case Task::ConvertH3TPLToJson:
            {
                run(Task::LoadH3TPLRaw, recurse + 1);

                runMember(propertySerializeH3TPL);
                setOutput(m_outputs.m_h3tpl.m_json);
                runMember(writeJsonFromProperty);
            } break;
            case Task::ConvertJsonToH3TPL:
            {
                setInput(m_inputs.m_h3tpl.m_json);
                runMember(readJsonToProperty);

                runMember(propertyDeserializeH3TPL);

                run(Task::SaveH3TPLRaw, recurse + 1);
            } break;
            case Task::H3TPLRoundTripJson:
            {
                if (!m_settings.m_dumpUncompressedBuffers)
                    throw std::runtime_error("You need to set dumpUncompressedBuffers");

                run(Task::ConvertH3TPLToJson, recurse + 1);
                safeCopy(m_settings.m_outputs.m_h3tpl.m_json, m_settings.m_inputs.m_h3tpl.m_json);
                run(Task::ConvertJsonToH3TPL, recurse + 1);

                setInput(m_inputs.m_h3tpl.m_uncompressedBinary);
                setOutput(m_outputs.m_h3tpl.m_uncompressedBinary);
                run(Task::CheckBinaryInputOutputEquality, recurse + 1);
            } break;
            case Task::H3TPLRoundTripFH:
            {
                if (!m_settings.m_dumpUncompressedBuffers)
                    throw std::runtime_error("You need to set dumpUncompressedBuffers");

                run(Task::H3TPLToFHTpl, recurse + 1);
                safeCopy(m_settings.m_outputs.m_fhMap, m_settings.m_inputs.m_fhMap);
                run(Task::FHTplToH3TPL, recurse + 1);

                setInput(m_inputs.m_h3tpl.m_json);
                setOutput(m_outputs.m_h3tpl.m_json);
                run(Task::CheckJsonInputOutputEquality, recurse + 1);

                setInput(m_inputs.m_h3tpl.m_uncompressedBinary);
                setOutput(m_outputs.m_h3tpl.m_uncompressedBinary);
                run(Task::CheckBinaryInputOutputEquality, recurse + 1);
            } break;

                // ----------------------------------------

            // high-level
            case Task::LoadFHTpl:
            {
                setInput(m_inputs.m_fhTemplate);
                runMember(readJsonToProperty);
                runMember(propertyDeserializeFHTpl);
            } break;
            case Task::SaveFHTpl:
            {
                runMember(propertySerializeFHTpl);
                setOutput(m_outputs.m_fhTemplate);
                runMember(writeJsonFromProperty);
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
            case Task::LoadFolder:
            {
                setInput(m_inputs.m_folder);
                m_folder.m_root = m_inputFilename;
                m_folder.read();
            } break;
            case Task::SaveFolder:
            {
                setOutput(m_outputs.m_folder);
                m_folder.m_root = m_outputFilename;
                m_folder.write();
            } break;

            // convenience calls
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
            case Task::FHMapToH3SVG:
            {
                run(Task::LoadFH, recurse + 1);
                run(Task::SaveH3SVG, recurse + 1);
            } break;
            case Task::H3SVGToFHMap:
            {
                run(Task::LoadH3SVG, recurse + 1);
                run(Task::SaveFH, recurse + 1);
            } break;
            case Task::FHTplToH3TPL:
            {
                run(Task::LoadFHTpl, recurse + 1);
                run(Task::SaveH3TPL, recurse + 1);
            } break;
            case Task::H3TPLToFHTpl:
            {
                run(Task::LoadH3TPL, recurse + 1);
                run(Task::SaveFHTpl, recurse + 1);
            } break;

            case H3CToFolder:
            {
                run(Task::LoadH3C, recurse + 1);
                run(Task::ConvertH3CToFolderList, recurse + 1);
                run(Task::SaveFolder, recurse + 1);
            } break;
            case Task::GenerateFHMap:
            {
                run(Task::LoadFHTpl, recurse + 1);

                runMember(generateFHMapFromFHTpl);

                run(Task::SaveFH, recurse + 1);
            } break;

            // utilities
            case Task::CheckBinaryInputOutputEquality:
            {
                checkBinaryInputOutputEquality();
            } break;
            case Task::CheckJsonInputOutputEquality:
            {
                checkJsonInputOutputEquality();
            } break;
        }
    }
    catch (RoundTripException&) {
        throw;
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
    m_mainFile.m_filename = m_inputFilename;
    m_mainFile.readBinaryBufferData();
    m_logOutput << m_currentIndent << "Read " << m_mainFile.m_binaryBuffer.size() << " bytes from: " << Mernel::path2string(m_inputFilename) << '\n';
}

void MapConverter::writeBinaryBufferData()
{
    m_logOutput << m_currentIndent << "Write " << m_mainFile.m_binaryBuffer.size() << " bytes to: " << Mernel::path2string(m_outputFilename) << '\n';
    m_mainFile.m_filename = m_outputFilename;
    m_mainFile.writeBinaryBufferData();
}

void MapConverter::writeBinaryBufferDataAsUncompressed()
{
    m_logOutput << m_currentIndent << "Write " << m_mainFile.m_binaryBuffer.size() << " bytes to: " << Mernel::path2string(m_outputFilename) << '\n';
    m_mainFile.m_filename = m_outputFilename;
    m_mainFile.writeBinaryBufferDataAsUncompressed();
}

void MapConverter::readJsonToProperty()
{
    m_logOutput << m_currentIndent << "Read: " << Mernel::path2string(m_inputFilename) << '\n';
    m_mainFile.m_filename = m_inputFilename;
    m_mainFile.readJsonToProperty();
}

void MapConverter::writeJsonFromProperty()
{
    m_logOutput << m_currentIndent << "Write: " << Mernel::path2string(m_outputFilename) << '\n';
    m_mainFile.m_filename = m_outputFilename;
    m_mainFile.writeJsonFromProperty();
}

void MapConverter::detectCompression()
{
    m_mainFile.detectCompression();
}

void MapConverter::uncompressRaw()
{
    m_mainFile.uncompressRaw();
}

void MapConverter::compressRaw()
{
    m_mainFile.compressRaw();
}

void MapConverter::uncompressRawParts()
{
    m_mainFile.splitCompressedDataByOffsets();
    m_mainFile.uncompressRawParts();
}

void MapConverter::compressRawParts()
{
    m_mainFile.compressRawParts();
}

void MapConverter::binaryDeserializeH3M()
{
    if (m_mainFile.m_rawState != RawState::Uncompressed)
        throw std::runtime_error("Buffer needs to be in Uncompressed state.");

    ByteOrderBuffer           bobuffer(m_mainFile.m_binaryBuffer);
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
    m_mainFile.m_binaryBuffer = {};
    ByteOrderBuffer           bobuffer(m_mainFile.m_binaryBuffer);
    ByteOrderDataStreamWriter writer(bobuffer, ByteOrderDataStream::s_littleEndian);

    writer << m_mapH3M;

    m_mainFile.m_rawState = RawState::Uncompressed;
}

void MapConverter::propertySerializeH3M()
{
    m_mapH3M.toJson(m_mainFile.m_json);
}

void MapConverter::propertyDeserializeH3M()
{
    m_mapH3M.fromJson(m_mainFile.m_json);
}

void MapConverter::binaryDeserializeH3SVG()
{
    if (m_mainFile.m_rawState != RawState::Uncompressed)
        throw std::runtime_error("Buffer needs to be in Uncompressed state.");

    ByteOrderBuffer           bobuffer(m_mainFile.m_binaryBuffer);
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
    m_mainFile.m_binaryBuffer = {};
    ByteOrderBuffer           bobuffer(m_mainFile.m_binaryBuffer);
    ByteOrderDataStreamWriter writer(bobuffer, ByteOrderDataStream::s_littleEndian);

    writer << m_mapH3SVG;

    m_mainFile.m_rawState = RawState::Uncompressed;
}

void MapConverter::propertySerializeH3SVG()
{
    m_mapH3SVG.toJson(m_mainFile.m_json);
}

void MapConverter::propertyDeserializeH3SVG()
{
    m_mapH3SVG.fromJson(m_mainFile.m_json);
}

void MapConverter::binaryDeserializeH3C()
{
    if (m_mainFile.m_rawState != RawState::Uncompressed)
        throw std::runtime_error("Buffer needs to be in Uncompressed state.");

    ByteOrderBuffer           bobuffer(m_mainFile.m_binaryParts[0]);
    ByteOrderDataStreamReader reader(bobuffer, ByteOrderDataStream::s_littleEndian);

    m_mapH3C.m_scenarios.clear();
    for (size_t i = 1; i < m_mainFile.m_binaryParts.size(); i++)
        m_mapH3C.m_scenarios.push_back({ .m_data = m_mainFile.m_binaryParts[i] });

    try {
        reader >> m_mapH3C;
    }
    catch (std::exception& ex) {
        throw std::runtime_error(ex.what() + std::string(", offset=") + std::to_string(bobuffer.getOffsetRead()));
    }
}

void MapConverter::binarySerializeH3C()
{
    m_mainFile.m_binaryBuffer = {};
    ByteOrderBuffer           bobuffer(m_mainFile.m_binaryBuffer);
    ByteOrderDataStreamWriter writer(bobuffer, ByteOrderDataStream::s_littleEndian);

    writer << m_mapH3C;

    m_mainFile.m_rawState = RawState::Uncompressed;
}

void MapConverter::propertySerializeH3C()
{
    m_mapH3C.toJson(m_mainFile.m_json);
}

void MapConverter::propertyDeserializeH3C()
{
    m_mapH3C.fromJson(m_mainFile.m_json);
}

void MapConverter::binaryDeserializeH3TPL()
{
    m_templateH3.readCSV(m_mainFile.m_csv);
}

void MapConverter::binarySerializeH3TPL()
{
    m_mainFile.m_csv = {};
    m_templateH3.writeCSV(m_mainFile.m_csv);
}

void MapConverter::propertySerializeH3TPL()
{
    m_templateH3.toJson(m_mainFile.m_json);
}

void MapConverter::propertyDeserializeH3TPL()
{
    m_templateH3.fromJson(m_mainFile.m_json);
}

void MapConverter::propertySerializeFH()
{
    m_mapFH.m_packedTileMap.packFromMap(m_mapFH.m_tileMap);
    m_mapFH.toJson(m_mainFile.m_json);
}

void MapConverter::propertyDeserializeFH()
{
    Core::GameVersion version = Core::GameVersion::SOD;
    if (m_mainFile.m_json["format"].getScalar().toString().starts_with("HOTA")) {
        version = Core::GameVersion::HOTA;
        if (m_mainFile.m_json["config"].contains("hotaVersion") && m_mainFile.m_json["config"]["hotaVersion"].contains("ver1")) {
            auto majorVersion = m_mainFile.m_json["config"]["hotaVersion"]["ver1"].getScalar().toInt();
            if (majorVersion >= 5)
                version = Core::GameVersion::HOTA_FACTORY;
        }
    }
    m_mapFH.m_database = m_databaseContainer->getDatabase(version);
    assert(m_mapFH.m_database);
    m_mapFH.fromJson(m_mainFile.m_json);
}

void MapConverter::propertySerializeFHTpl()
{
    // @todo:
    propertySerializeFH();
}

void MapConverter::propertyDeserializeFHTpl()
{
    // @todo:
    propertyDeserializeFH();
}

void MapConverter::convertFHtoH3M()
{
    Core::GameVersion version = Core::GameVersion::SOD;
    if (m_mapFH.m_format >= FHMap::MapFormat::HOTA1 && m_mapFH.m_format <= FHMap::MapFormat::HOTA3)
        version = Core::GameVersion::HOTA;
    if (m_mapFH.m_format == FHMap::MapFormat::HOTA3 && m_mapFH.m_config.m_hotaVersion.m_ver1 >= 5)
        version = Core::GameVersion::HOTA_FACTORY;
    m_mapFH.m_database = m_databaseContainer->getDatabase(version);
    convertFH2H3M(m_mapFH, m_mapH3M);
}

void MapConverter::convertH3MtoFH()
{
    Core::GameVersion version = Core::GameVersion::SOD;
    if (m_mapH3M.m_format >= MapFormat::HOTA1 && m_mapH3M.m_format <= MapFormat::HOTA3)
        version = Core::GameVersion::HOTA;
    if (m_mapH3M.m_format == MapFormat::HOTA3 && m_mapH3M.m_hotaVer.m_ver1 >= 5)
        version = Core::GameVersion::HOTA_FACTORY;

    m_mapFH.m_database = m_databaseContainer->getDatabase(version);
    convertH3M2FH(m_mapH3M, m_mapFH);
}

void MapConverter::convertFHtoH3SVG()
{
    throw std::runtime_error("Unsupported yet.");
}

void MapConverter::convertH3SVGtoFH()
{
    throw std::runtime_error("Unsupported yet.");
}

void MapConverter::convertFHTpltoH3TPL()
{
    throw std::runtime_error("Unsupported yet.");
}

void MapConverter::convertH3TPLtoFHTpl()
{
    throw std::runtime_error("Unsupported yet.");
}

void MapConverter::generateFHMapFromFHTpl()
{
    auto rng = m_rngFactory->create();
    if (m_templateSettings.m_seed)
        m_mapFH.m_seed = m_templateSettings.m_seed;

    Core::GameVersion version = Core::GameVersion::SOD;
    if (m_mapFH.m_format >= FHMap::MapFormat::HOTA1 && m_mapFH.m_format <= FHMap::MapFormat::HOTA3)
        version = Core::GameVersion::HOTA;
    if (m_mapH3M.m_format == MapFormat::HOTA3 && m_mapH3M.m_hotaVer.m_ver1 >= 5)
        version = Core::GameVersion::HOTA_FACTORY;

    m_mapFH.m_database = m_databaseContainer->getDatabase(version);

    if (!m_templateSettings.m_rngUserSettings.empty()) {
        m_logOutput << m_currentIndent << "Read: " << Mernel::path2string(m_templateSettings.m_rngUserSettings) << '\n';
        std::string buffer       = Mernel::readFileIntoBuffer(m_templateSettings.m_rngUserSettings);
        auto        settingsJson = Mernel::readJsonFromBuffer(buffer);

        m_mapFH.applyRngUserSettings(settingsJson);
    }
    m_mapFH.rescaleToUserSize();

    rng->setSeed(m_mapFH.m_seed);

    FHTemplateProcessor converter(m_mapFH,
                                  rng.get(),
                                  m_logOutput,
                                  m_templateSettings.m_stopAfterStage,
                                  m_templateSettings.m_showDebugStage,
                                  m_templateSettings.m_tileFilter,
                                  m_templateSettings.m_stopAfterHeat,
                                  m_templateSettings.m_extraLogging);
    converter.run();
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
        throw RoundTripException("Failed input == output");
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

    throw RoundTripException("Failed input == output");
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
    m_output << "end " << m_currentTask << " (" << m_timer.elapsedUS() << " us.)\n";
}

}
