/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MapConverter.hpp"

#include "Reflection/EnumTraits.hpp"

#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"
#include "FileIOUtils.hpp"
#include "FileFormatJson.hpp"
#include "Compression.hpp"

#include "FH2H3M.hpp"
#include "H3M2FH.hpp"

#include <iostream>

namespace FreeHeroes {

namespace Core::Reflection {
template<>
inline constexpr const auto EnumTraits::s_valueMapping<MapConverter::Task> = EnumTraits::make(
    MapConverter::Task::Invalid,
    "Invalid",
    MapConverter::Task::Invalid,

    "ReadH3MRawData",
    MapConverter::Task::ReadH3MRawData,
    "WriteH3MRawData",
    MapConverter::Task::WriteH3MRawData,
    "DumpH3MRawDataAsUncompressedInput",
    MapConverter::Task::DumpH3MRawDataAsUncompressedInput,
    "DumpH3MRawDataAsUncompressedOutput",
    MapConverter::Task::DumpH3MRawDataAsUncompressedOutput,

    "ReadJsonToProperty",
    MapConverter::Task::ReadJsonToProperty,
    "WriteJsonFromProperty",
    MapConverter::Task::WriteJsonFromProperty,

    "DetectCompression",
    MapConverter::Task::DetectCompression,
    "UncompressRaw",
    MapConverter::Task::UncompressRaw,
    "CompressRaw",
    MapConverter::Task::CompressRaw,

    "ReadH3MRawToLegacyObject",
    MapConverter::Task::ReadH3MRawToLegacyObject,
    "WriteH3MFromLegacyObject",
    MapConverter::Task::WriteH3MRawFromLegacyObject,
    "SerializeLegacyObjectToProperty",
    MapConverter::Task::SerializeLegacyObjectToProperty,
    "DeserializeLegacyObjectFromProperty",
    MapConverter::Task::DeserializeLegacyObjectFromProperty,
    "SerializeFHObjectToProperty",
    MapConverter::Task::SerializeFHObjectToProperty,
    "DeserializeFHObjectFromProperty",
    MapConverter::Task::DeserializeFHObjectFromProperty,
    "ConstructH3MFromFH",
    MapConverter::Task::ConstructH3MFromFH,
    "ConstructFHFromH3M",
    MapConverter::Task::ConstructFHFromH3M,
    "CheckH3MInputOutputEquality",
    MapConverter::Task::CheckH3MInputOutputEquality,

    "ConvertH3MToJson",
    MapConverter::Task::ConvertH3MToJson,
    "ConvertJsonToH3M",
    MapConverter::Task::ConvertJsonToH3M,
    "FHMapToH3M",
    MapConverter::Task::FHMapToH3M,
    "H3MToFHMap",
    MapConverter::Task::H3MToFHMap,
    "H3MRoundTripJson",
    MapConverter::Task::H3MRoundTripJson,
    "H3MRoundTripFH",
    MapConverter::Task::H3MRoundTripFH);

}

std::string taskToString(MapConverter::Task task)
{
    auto str = Core::Reflection::EnumTraits::enumToString(task);
    return std::string(str.begin(), str.end());
}
MapConverter::Task stringToTask(const std::string& str)
{
    return Core::Reflection::EnumTraits::stringToEnum<MapConverter::Task>({ str.c_str(), str.size() });
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

bool MapConverter::run(Task task, int recurse) noexcept
{
    try {
        for (int r = 0; r < recurse; ++r)
            m_logOutput << "  ";
        m_logOutput << "run " << taskToString(task) << "\n";
        auto checkFilename = [this](const std::string& name, const Core::std_path& path, bool checkExists) -> bool {
            if (path.empty()) {
                m_logOutput << "Path '" << name << "' must not be empty\n";
                return false;
            }
            if (checkExists && !Core::std_fs::exists(path)) {
                m_logOutput << "Path '" << path << "' must exist!\n";
                return false;
            }
            return true;
        };
        switch (task) {
            case Task::Invalid:
            {
                m_logOutput << "Can't execute invalid task.\n";
                return false;
            }
                // -------------------------- Binary I/O ----------------------------------
            case Task::ReadH3MRawData:
            {
                m_rawState = RawState::Undefined;
                if (!checkFilename("h3mInput", m_settings.m_h3mInput, true))
                    return false;

                if (!Core::readFileIntoBuffer(m_settings.m_h3mInput, m_h3mraw)) {
                    m_logOutput << "Failed to read " << m_settings.m_h3mInput << ".\n";
                    return false;
                }
                m_rawState = RawState::Compressed;
            } break;
            case Task::WriteH3MRawData:
            {
                if (!checkFilename("h3mOutput", m_settings.m_h3mOutput, false))
                    return false;
                if (m_rawState != RawState::Compressed) {
                    m_logOutput << "Buffer need to be in Compressed state.\n";
                    return false;
                }

                if (!Core::writeFileFromBuffer(m_settings.m_h3mOutput, m_h3mraw)) {
                    m_logOutput << "Failed to write to " << m_settings.m_h3mOutput << ".\n";
                    return false;
                }

            } break;

            case Task::DumpH3MRawDataAsUncompressedInput:
            {
                if (!checkFilename("h3mUncompressedInput", m_settings.m_h3mUncompressedInput, false))
                    return false;
                if (m_rawState != RawState::Uncompressed) {
                    m_logOutput << "Buffer need to be in Uncompressed state.\n";
                    return false;
                }

                if (!Core::writeFileFromBuffer(m_settings.m_h3mUncompressedInput, m_h3mraw)) {
                    m_logOutput << "Failed to write to " << m_settings.m_h3mUncompressedInput << ".\n";
                    return false;
                }
            } break;
            case Task::DumpH3MRawDataAsUncompressedOutput:
            {
                if (!checkFilename("h3mUncompressedOutput", m_settings.m_h3mUncompressedOutput, false))
                    return false;
                if (m_rawState != RawState::Uncompressed) {
                    m_logOutput << "Buffer need to be in Uncompressed state.\n";
                    return false;
                }

                if (!Core::writeFileFromBuffer(m_settings.m_h3mUncompressedOutput, m_h3mraw)) {
                    m_logOutput << "Failed to write to " << m_settings.m_h3mUncompressedOutput << ".\n";
                    return false;
                }
            } break;

                // -------------------------- Text I/O ----------------------------------
            case Task::ReadJsonToProperty:
            {
                if (!checkFilename("jsonInput", m_settings.m_jsonInput, true))
                    return false;
                std::string buffer;
                if (!Core::readFileIntoBuffer(m_settings.m_jsonInput, buffer)) {
                    m_logOutput << "Failed to read " << m_settings.m_jsonInput << ".\n";
                    return false;
                }
                if (!Core::readJsonFromBuffer(buffer, m_json)) {
                    m_logOutput << "Failed to parse JSON " << m_settings.m_jsonInput << ".\n";
                    return false;
                }

            } break;
            case Task::WriteJsonFromProperty:
            {
                if (!checkFilename("jsonOutput", m_settings.m_jsonOutput, false))
                    return false;

                std::string buffer;
                if (!Core::writeJsonToBuffer(buffer, m_json)) {
                    m_logOutput << "Failed to serialize JSON tree to byte buffer for " << m_settings.m_jsonOutput << ".\n";
                    return false;
                }

                if (!Core::writeFileFromBuffer(m_settings.m_jsonOutput, buffer)) {
                    m_logOutput << "Failed to write to " << m_settings.m_jsonOutput << ".\n";
                    return false;
                }

            } break;

                // -------------------------- Compression tasks ----------------------------------
            case Task::DetectCompression:
            {
                m_compressionMethod = CompressionMethod::Undefined;
                if (m_rawState != RawState::Compressed) {
                    m_logOutput << "Buffer need to be in Compressed state.\n";
                    return false;
                }

                // todo: real detection of different methods!
                m_compressionMethod = CompressionMethod::Gzip;
            } break;
            case Task::UncompressRaw:
            {
                if (m_rawState != RawState::Compressed) {
                    m_logOutput << "Buffer need to be in Compressed state.\n";
                    return false;
                }
                if (m_compressionMethod == CompressionMethod::Undefined) {
                    m_logOutput << "CompressionMethod need to be defined (or detected).\n";
                    return false;
                }
                if (m_compressionMethod == CompressionMethod::NoCompression) {
                    m_rawState = RawState::Uncompressed;
                    return true;
                }
                if (m_compressionMethod != CompressionMethod::Gzip) {
                    assert("Invalid compression method");
                    return false;
                }

                ByteArrayHolder out;
                Core::uncompressDataBuffer(m_h3mraw, out, { .m_type = Core::CompressionType::Gzip }); // throws;
                m_h3mraw = std::move(out);

                m_rawState = RawState::Uncompressed;
            } break;
            case Task::CompressRaw:
            {
                if (m_rawState != RawState::Uncompressed) {
                    m_logOutput << "Buffer need to be in Uncompressed state.\n";
                    return false;
                }
                if (m_compressionMethod == CompressionMethod::Undefined) {
                    m_logOutput << "CompressionMethod need to be defined.\n";
                    return false;
                }
                if (m_compressionMethod == CompressionMethod::NoCompression) {
                    m_rawState = RawState::Compressed;
                    return true;
                }
                if (m_compressionMethod != CompressionMethod::Gzip) {
                    assert("Invalid compression method");
                    return false;
                }

                ByteArrayHolder out;
                Core::compressDataBuffer(m_h3mraw, out, { .m_type = Core::CompressionType::Gzip }); // throws;
                m_h3mraw = std::move(out);

                m_rawState = RawState::Compressed;
            } break;

                // -------------------------- Primitive tasks ----------------------------------
            case Task::ReadH3MRawToLegacyObject:
            {
                if (m_rawState != RawState::Uncompressed) {
                    m_logOutput << "Buffer must be in uncomressed state.\n";
                    return false;
                }

                ByteOrderBuffer           bobuffer(m_h3mraw);
                ByteOrderDataStreamReader reader(bobuffer, ByteOrderDataStream::createByteorderMask(ORDER_LE, ORDER_LE, ORDER_LE));

                try {
                    reader >> m_mapLegacy;
                }
                catch (std::exception& ex) {
                    throw std::runtime_error(ex.what() + std::string(", offset=") + std::to_string(bobuffer.getOffsetRead()));
                }
            } break;

            case Task::WriteH3MRawFromLegacyObject:
            {
                m_h3mraw = {};
                ByteOrderBuffer           bobuffer(m_h3mraw);
                ByteOrderDataStreamWriter writer(bobuffer, ByteOrderDataStream::createByteorderMask(ORDER_LE, ORDER_LE, ORDER_LE));

                writer << m_mapLegacy;

                m_rawState = RawState::Uncompressed;
            } break;

            case Task::SerializeLegacyObjectToProperty:
            {
                m_mapLegacy.toJson(m_json);
            } break;
            case Task::DeserializeLegacyObjectFromProperty:
            {
                m_mapLegacy.fromJson(m_json);
            } break;
            case Task::SerializeFHObjectToProperty:
            {
                m_mapFH.toJson(m_json);
            } break;
            case Task::DeserializeFHObjectFromProperty:
            {
                if (m_json["version"].getScalar().toString() == "HOTA")
                    m_mapFH.m_version = Core::GameVersion::HOTA;
                else
                    m_mapFH.m_version = Core::GameVersion::SOD;
                m_mapFH.fromJson(m_json, m_databaseContainer->getDatabase(m_mapFH.m_version));

            } break;
            case Task::ConstructH3MFromFH:
            {
                auto rnd = m_rngFactory->create();
                rnd->setSeed(m_mapFH.m_seed);
                convertFH2H3M(m_mapFH, m_mapLegacy, m_databaseContainer->getDatabase(m_mapFH.m_version), rnd.get());
            } break;
            case Task::ConstructFHFromH3M:
            {
                if (m_mapLegacy.m_format >= MapFormat::HOTA1)
                    m_mapFH.m_version = Core::GameVersion::HOTA;
                else
                    m_mapFH.m_version = Core::GameVersion::SOD;
                convertH3M2FH(m_mapLegacy, m_mapFH, m_databaseContainer->getDatabase(m_mapFH.m_version));
            } break;

            case Task::CheckH3MInputOutputEquality:
            {
                std::string bufferIn, bufferOut;
                Core::readFileIntoBuffer(m_settings.m_h3mUncompressedInput, bufferIn);
                Core::readFileIntoBuffer(m_settings.m_h3mUncompressedOutput, bufferOut);
                const bool result = bufferIn == bufferOut;
                m_logOutput << "Round trip result: " << (result ? "PASSED" : "FAILED") << ", h3mInput size=" << bufferIn.size() << ", h3mOutput size=" << bufferOut.size() << "\n";
                if (!result) {
                    auto minSize = std::min(bufferIn.size(), bufferOut.size());
                    bufferIn.resize(minSize);
                    bufferOut.resize(minSize);
                    const bool commonEqual = bufferIn == bufferOut;
                    m_logOutput << "Common part is: " << (commonEqual ? "EQUAL" : "DIFFERENT") << "\n";
                    if (!commonEqual) {
                        int maxDiffCounter = 10;
                        for (size_t i = 0; i < minSize; ++i) {
                            if (bufferIn[i] != bufferOut[i]) {
                                if (maxDiffCounter-- > 0)
                                    m_logOutput << "difference at [" << i << " / 0x" << std::hex << std::setfill('0') << i << "], in: 0x" << std::setw(2) << int(uint8_t(bufferIn[i]))
                                                << ", out: 0x" << std::setw(2) << int(uint8_t(bufferOut[i])) << "\n"
                                                << std::dec << std::setfill(' ');
                            }
                        }
                    }
                }
                return result;

            } break;

                //  -------------------------- High-level tasks ----------------------------------
            case Task::ConvertH3MToJson:
            {
                return true
                       && run(Task::ReadH3MRawData, recurse + 1)
                       && run(Task::DetectCompression, recurse + 1)
                       && run(Task::UncompressRaw, recurse + 1)
                       && run(Task::DumpH3MRawDataAsUncompressedInput, recurse + 1)
                       && run(Task::ReadH3MRawToLegacyObject, recurse + 1)
                       && run(Task::SerializeLegacyObjectToProperty, recurse + 1)
                       && run(Task::WriteJsonFromProperty, recurse + 1);
            } break;
            case Task::ConvertJsonToH3M:
            {
                return true
                       && run(Task::ReadJsonToProperty, recurse + 1)
                       && run(Task::DeserializeLegacyObjectFromProperty, recurse + 1)
                       && run(Task::WriteH3MRawFromLegacyObject, recurse + 1)
                       && run(Task::DumpH3MRawDataAsUncompressedOutput, recurse + 1)
                       //&& [this]() { m_compressionMethod = CompressionMethod::Gzip; return true; }()
                       && run(Task::CompressRaw, recurse + 1)
                       && run(Task::WriteH3MRawData, recurse + 1);
            } break;
            case Task::FHMapToH3M:
            {
                return true
                       && run(Task::ReadJsonToProperty, recurse + 1)
                       && run(Task::DeserializeFHObjectFromProperty, recurse + 1)
                       && run(Task::ConstructH3MFromFH, recurse + 1)
                       && run(Task::SerializeLegacyObjectToProperty, recurse + 1)
                       // && run(Task::WriteJsonFromProperty, recurse + 1)
                       && run(Task::WriteH3MRawFromLegacyObject, recurse + 1)
                       && run(Task::DumpH3MRawDataAsUncompressedOutput, recurse + 1)
                       && [this]() { m_compressionMethod = CompressionMethod::Gzip; return true; }()
                       && run(Task::CompressRaw, recurse + 1)
                       && run(Task::WriteH3MRawData, recurse + 1);
            } break;
            case Task::H3MToFHMap:
            {
                return true
                       && run(Task::ReadH3MRawData, recurse + 1)
                       && run(Task::DetectCompression, recurse + 1)
                       && run(Task::UncompressRaw, recurse + 1)
                       && run(Task::DumpH3MRawDataAsUncompressedInput, recurse + 1)
                       && run(Task::ReadH3MRawToLegacyObject, recurse + 1)
                       && run(Task::ConstructFHFromH3M, recurse + 1)
                       && run(Task::SerializeFHObjectToProperty, recurse + 1)
                       && run(Task::WriteJsonFromProperty, recurse + 1);
                ;
            } break;
            case Task::H3MRoundTripJson:
            {
                if (m_settings.m_jsonInput != m_settings.m_jsonOutput) {
                    m_logOutput << "You must set jsonInput equal to jsonOutput to perform roundtrip check.\n";
                    return false;
                }

                return true
                       && run(Task::ConvertH3MToJson, recurse + 1)
                       && run(Task::ConvertJsonToH3M, recurse + 1)
                       && run(Task::CheckH3MInputOutputEquality, recurse + 1);
            } break;
            case Task::H3MRoundTripFH:
            {
                if (m_settings.m_jsonInput != m_settings.m_jsonOutput) {
                    m_logOutput << "You must set jsonInput equal to jsonOutput to perform roundtrip check.\n";
                    return false;
                }

                return true
                       && run(Task::H3MToFHMap, recurse + 1)
                       && run(Task::FHMapToH3M, recurse + 1)
                       && run(Task::CheckH3MInputOutputEquality, recurse + 1);
            } break;
        }
    }
    catch (std::exception& ex) {
        m_logOutput << "Exception thrown during execution of task '" << taskToString(task) << "': " << ex.what() << ".\n";
        return false;
    }

    return true;
}

}
