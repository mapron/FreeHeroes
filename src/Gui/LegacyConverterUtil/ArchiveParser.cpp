/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ArchiveParser.hpp"

#include "SpriteSerialization.hpp"
#include "SpriteParserLegacy.hpp"

#include "KnownResources.hpp"
#include "CompressionUtils.hpp"
#include "MediaConverter.hpp"

#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/FileIOUtils.hpp"

#include "MernelPlatform/Logger.hpp"

#include <QTextCodec>
#include <QDataStream>

#include <fstream>
#include <iostream>
#include <vector>
#include <set>

namespace FreeHeroes::Conversion {

using namespace Core;
using namespace Gui;
using namespace Mernel;

namespace {

QString readString(QDataStream& ds, size_t size)
{
    QByteArray t;
    t.resize(size);

    ds.readRawData(t.data(), size);
    return QString::fromUtf8(t.data());
}

std::string toLower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](char c) { return tolower(c); });
    return str;
}

struct Record {
    QString  filename;
    uint32_t offset;
    uint32_t fullSize;
    uint32_t compressedSize;
};

struct HdatRecord {
    QString     fname;
    QString     fname2;
    QStringList strList;

    QByteArray        data;
    QVector<uint32_t> addParams;
};

const QMap<ResourceType, std_path> defaultSubfolders{
    { ResourceType::Sprite, "pcx" },
    { ResourceType::Sound, "effects" },
    { ResourceType::Music, "music" },
    { ResourceType::Video, "video" },
    // { ResourceType::Other, "txt" },
};

// @todo: do we need: .fnt, .pal, .h3c, .ifr, .xmi ? .h3c probably yes, campaigns some day!
ResourceType guessType(const std_path& resourceExt, bool* isPCX)
{
    if (isPCX)
        *isPCX = false;
    if (resourceExt == ".def" || resourceExt == ".d32") {
        return ResourceType::Sprite;
    } else if (resourceExt == ".pcx" || resourceExt == ".p32") {
        if (isPCX)
            *isPCX = true;
        return ResourceType::Sprite;
    } else if (resourceExt == ".wav") {
        return ResourceType::Sound;
    } else if (resourceExt == ".mp3") {
        return ResourceType::Music;
    } else if (resourceExt == ".bik" || resourceExt == ".smk") {
        return ResourceType::Video;
    } else if (resourceExt == ".txt") {
        //return ResourceType::Other;
    }
    return ResourceType::Invalid;
}

void ensureDirExistence(const std_path& path)
{
    const std_path dir = path; //std_fs::is_directory(path) ? path : path.parent_path();
    if (!std_fs::exists(dir))
        std_fs::create_directories(dir);
}

}

ArchiveParser::ArchiveParser(KnownResources&    knownResources,
                             QSet<ResourceType> requiredTypes,
                             bool               overrideExisting,
                             bool               keepTmp,
                             ExtractCallbackInc extractCallbackInc)
    : m_knownResources(knownResources)
    , m_requiredTypes(std::move(requiredTypes))
    , m_overrideExisting(overrideExisting)
    , m_keepTmp(keepTmp)
    , m_extractCallbackInc(std::move(extractCallbackInc))
{
}

int ArchiveParser::estimateExtractCount(const ExtractionList& extractionList)
{
    int              total = 0;
    CallbackInserter cbList, ppList;
    for (const auto& task : extractionList) {
        int count = 0;
        proceed(task, cbList, ppList, &count);
        total += count;
    }
    return total;
}

void ArchiveParser::prepareExtractTasks(const ExtractionList& extractionList, CallbackInserter& conversion, CallbackInserter& postprocess)
{
    for (const auto& task : extractionList) {
        proceed(task, conversion, postprocess, nullptr);
    }
}

bool ArchiveParser::proceed(const ExtractionTask& task, CallbackInserter& conversion, CallbackInserter& postprocess, int* estimate)
{
    if (!estimate) {
        ensureDirExistence(task.destResourceRoot);
    }

    m_file.close();
    if (!task.srcFilename.empty()) {
        m_file.setFileName(stdPath2QString(task.srcRoot / task.srcFilename));

        if (!m_file.open(QIODevice::ReadOnly))
            return false;

        if (!estimate)
            Logger(Logger::Info) << "Extracting :" << task.srcFilename;

        m_ds.setDevice(&m_file);
        m_ds.setByteOrder(QDataStream::LittleEndian);
    }

    // clang-format off
    switch(task.type) {
        case TaskType::LOD: return extractLOD(task, conversion, postprocess, estimate);
        case TaskType::SND: return extractSND(task, conversion, estimate);
        case TaskType::VID: return extractVID(task, conversion, estimate);
        case TaskType::HDAT: return extractHDAT(task, conversion, estimate);
        case TaskType::MusicCopy: return copyMusic(task, conversion, estimate);
        case TaskType::DefCopy: return copyDef(task, conversion, estimate);
    }
    // clang-format on
    return false;
}

bool ArchiveParser::copyMusic(const ExtractionTask& task, CallbackInserter& conversion, int* estimate)
{
    if (!m_requiredTypes.contains(ResourceType::Music))
        return false;

    (void) conversion;
    const std_path dest = task.destResourceRoot / "Music";
    ensureDirExistence(dest);

    for (const auto& p : std_fs::directory_iterator(task.srcRoot)) {
        std::error_code ec;
        const std_path  filename  = toLower(path2string(p.path().filename()));
        const std_path  resouceId = filename.stem();
        if (needSkipResource(task, resouceId, ResourceType::Music))
            continue;

        if (estimate) {
            (*estimate)++;
            continue;
        }
        m_extractCallbackInc();

        std_fs::copy(p.path(), dest / filename, std_fs::copy_options::overwrite_existing, ec);
        //if (!ec)
        //    task.resources->registerResource(ResourceMedia{ ResourceType::Music, path2string(resouceId), "Music/", path2string(filename), {} });
    }
    return true;
}

bool ArchiveParser::copyDef(const ArchiveParser::ExtractionTask& task, CallbackInserter& conversion, int* estimate)
{
    if (!m_requiredTypes.contains(ResourceType::Sprite))
        return false;

    // (void)conversion;
    const std_path dest = task.destResourceRoot;
    ensureDirExistence(dest);

    for (const auto& p : std_fs::directory_iterator(task.srcRoot)) {
        std::error_code   ec;
        const std_path    srcFilePath = p.path();
        const std_path    filename    = toLower(path2string(srcFilePath.filename()));
        const std_path    resourceId  = filename.stem();
        const std::string ext         = path2string(filename.extension());
        const bool        isBmp       = ext == ".bmp";
        const bool        isDef       = ext == ".def";
        if (!isBmp && !isDef)
            continue;
        if (needSkipResource(task, resourceId, ResourceType::Sprite))
            continue;

        if (estimate) {
            (*estimate)++;
            continue;
        }
        const std::string destName     = makeJsonFilename(path2string(resourceId));
        const std_path    destFilePath = dest / destName;
        m_extractCallbackInc();

        auto conversionRoutine = [isBmp, srcFilePath, destFilePath] {
            SpritePtr sprite;
            if (!isBmp) {
                sprite = loadSpriteLegacy(srcFilePath);
            } else {
                sprite = loadBmp(srcFilePath);
            }

            saveSprite(sprite, destFilePath);
        };
        conversion(conversionRoutine);

        //task.resources->registerResource(ResourceMedia{ ResourceType::Sprite, path2string(resourceId), "", destName, {} });
    }
    return true;
}

bool ArchiveParser::extractLOD(const ExtractionTask& task, CallbackInserter& conversion, CallbackInserter& postprocess, int* estimate)
{
    uint32_t totalFiles = 0;
    m_ds.skipRawData(8);

    m_ds >> totalFiles;

    if (!totalFiles)
        return false;

    m_ds.skipRawData(80);

    QVector<Record> records(totalFiles);
    for (auto& rec : records) {
        uint32_t unk;
        rec.filename = readString(m_ds, 16).toLower();
        m_ds >> rec.offset >> rec.fullSize >> unk >> rec.compressedSize;
    }
    const auto destTmp = task.destResourceRoot / "tmp";
    ensureDirExistence(destTmp);

    std::vector<char> buffer;
    QList<std_path>   extractedResources;
    for (const auto& rec : records) {
        m_ds.device()->seek(rec.offset);
        const std_path resourceFilename = QString2stdPath(rec.filename);
        const std_path resourcePath     = destTmp / resourceFilename;
        const std_path resourceId       = resourceFilename.stem();
        const std_path resourceExt      = resourceFilename.extension();

        if (resourceId == "ovslot") // @todo: failing  assert(boundarySize.isEmpty() || frameBoundary == boundarySize);
            continue;

        if (needSkipResource(task, resourceId, resourceExt))
            continue;

        auto* knownResource = m_knownResources.find(path2string(resourceId));
        if (knownResource && needSkipResource(task, knownResource->newId, resourceExt))
            continue;
        bool               isPcx = false;
        const ResourceType type  = guessType(resourceExt, &isPcx);

        if (estimate) {
            if (type != ResourceType::Invalid)
                (*estimate)++;
            continue;
        }

        std::ofstream ofs(resourcePath, std::ios::binary | std::ios::trunc);
        if (rec.compressedSize > 0) {
            buffer.resize(rec.compressedSize);
            m_ds.readRawData(buffer.data(), rec.compressedSize);
            zlibUncompressFromBuffer(buffer.data(), rec.compressedSize, ofs);
        } else {
            buffer.resize(rec.fullSize);
            m_ds.readRawData(buffer.data(), rec.fullSize);
            ofs.write(buffer.data(), rec.fullSize);
        }
        ofs.close();
        extractedResources << resourceFilename;
        if (type != ResourceType::Invalid)
            m_extractCallbackInc();
    }
    if (estimate)
        return true;

    for (auto resourceFilename : extractedResources) {
        const std_path resourceId       = resourceFilename.stem();
        const std_path resourceExt      = resourceFilename.extension();
        std::string    mainResourceName = path2string(resourceFilename);
        const std_path srcFilePath      = destTmp / resourceFilename;

        std_path           subfolder;
        bool               isPcx = false;
        const ResourceType type  = guessType(resourceExt, &isPcx);

        subfolder = defaultSubfolders.value(type);

        std::string fullIdStr  = path2string(resourceId);
        std::string shortIdStr = fullIdStr;
        //std::string targetResourceIdStr = path2string(resourceId);
        const auto* knownResource = m_knownResources.find(fullIdStr);
        if (knownResource) {
            subfolder  = knownResource->destinationSubfolder;
            fullIdStr  = knownResource->newId;
            shortIdStr = knownResource->newId;
        }
        if (type == ResourceType::Sprite) {
            mainResourceName = makeJsonFilename(shortIdStr);
        }
        const std_path destRoot     = task.destResourceRoot / subfolder;
        const std_path destFilePath = destRoot / mainResourceName;

        if (type == ResourceType::Sprite) {
            auto conversionRoutine = [isPcx, srcFilePath, destFilePath, keepTmp = m_keepTmp] {
                SpritePtr sprite;
                if (!isPcx) {
                    sprite = loadSpriteLegacy(srcFilePath);
                } else {
                    sprite = loadPcx(srcFilePath);
                }
                saveSprite(sprite, destFilePath);
                if (!keepTmp) {
                    std_fs::remove(srcFilePath);
                }
            };

            if (knownResource && !knownResource->handlers.empty()) {
                auto ppRoutine = [knownResource, destFilePath, isHota = task.isHota, library = task.resources]() {
                    auto handlers = knownResource->handlers;

                    const std::vector<std::string> names{ "make_transparent", "unpack" };
                    for (const auto& name : names) {
                        if (handlers.contains(name + "_hota")) {
                            if (isHota) {
                                handlers[name] = handlers[name + "_hota"];
                            }
                            handlers.erase(name + "_hota");
                        }
                        if (handlers.contains(name + "_sod")) {
                            if (!isHota) {
                                handlers[name] = handlers[name + "_sod"];
                            }
                            handlers.erase(name + "_sod");
                        }
                    }
                    auto sprite   = loadSprite(destFilePath);
                    auto ppSprite = postProcessSprite(destFilePath, sprite, handlers, library);
                    saveSprite(ppSprite, destFilePath);
                };
                postprocess(ppRoutine);
            }
            conversion(conversionRoutine);
        }

        if (type == ResourceType::Invalid)
            std_fs::remove(srcFilePath);

        if (type == ResourceType::Invalid)
            continue;

        if (type != ResourceType::Sprite) {
            std_fs::create_directories(destRoot);
            std_fs::rename(srcFilePath, destFilePath);
        }

        //task.resources->registerResource(ResourceMedia{ type, fullIdStr, path2string(subfolder) + "/", mainResourceName, {} });
    }
    return true;
}

bool ArchiveParser::extractHDAT(const ExtractionTask& task, CallbackInserter& conversion, int* estimate)
{
    //if (!m_requiredTypes.contains(ResourceType::Other))
    //    return false;
    (void) conversion;

    uint32_t signature;
    m_ds >> signature;
    if (signature != 0x54414448) // 'HDAT'
        return false;

    uint32_t unknown1, recordsCount;
    m_ds >> unknown1 >> recordsCount;

    QTextCodec* defaultTextCodec = QTextCodec::codecForName("Windows-1251");

    auto readStr = [this, &defaultTextCodec]() -> QString {
        uint32_t len;
        m_ds >> len;
        QString    str;
        QByteArray data;
        data.resize(len);
        m_ds.readRawData(data.data(), len);
        str = defaultTextCodec->toUnicode(data);
        return str;
    };

    auto readFileRecord = [this, &readStr]() -> HdatRecord {
        HdatRecord rec;
        rec.fname  = readStr();
        rec.fname2 = readStr();
        uint32_t strCount;
        m_ds >> strCount;

        for (uint32_t i = 0; i < strCount; ++i) {
            QString str = readStr();
            rec.strList << str;
        }
        uint8_t one;
        m_ds >> one;
        if (one) {
            uint32_t blobSize;
            m_ds >> blobSize;
            rec.data.resize(blobSize);
            m_ds.readRawData(rec.data.data(), blobSize);
        }
        uint32_t someData;
        m_ds >> someData;
        rec.addParams.resize(someData);
        for (uint32_t i = 0; i < someData; ++i) {
            m_ds >> rec.addParams[i];
        }
        return rec;
    };

    QList<HdatRecord> records;
    for (uint32_t i = 0; i < recordsCount; ++i) {
        records << readFileRecord();
    }

    const std_path destFullpath = task.destResourceRoot / "json";
    ensureDirExistence(destFullpath);

    for (const auto& rec : records) {
        auto     outFileName = rec.fname.toStdString() + ".json";
        std_path p           = destFullpath / outFileName;
        if (std_fs::exists(p))
            continue;

        if (estimate) {
            (*estimate)++;
            continue;
        }
        m_extractCallbackInc();

        std::string  buffer;
        PropertyTree root;
        PropertyTree strings, intParams;
        strings.convertToList();
        intParams.convertToList();
        for (const auto& val : rec.strList)
            strings.append(PropertyTreeScalar(val.toStdString()));
        for (const auto& val : rec.addParams)
            intParams.append(PropertyTreeScalar(val));

        root["strings"]   = std::move(strings);
        root["intParams"] = std::move(intParams);
        root["binData"]   = PropertyTreeScalar(rec.data.toHex().toStdString());

        if (!Mernel::writeJsonToBuffer(buffer, root))
            return false;
        if (!Mernel::writeFileFromBuffer(p, buffer))
            return false;

        //task.resources->registerResource({ ResourceType::Other, rec.fname.toStdString(), "json/", outFileName, {} });
    }
    return true;
}

bool ArchiveParser::extractSND(const ExtractionTask& task, CallbackInserter& conversion, int* estimate)
{
    const ResourceType type = ResourceType::Sound;
    if (!m_requiredTypes.contains(type))
        return false;

    uint32_t recordsCount;
    m_ds >> recordsCount;

    MediaConverter conv;

    QVector<Record> records(recordsCount);
    for (auto& rec : records) {
        rec.filename = readString(m_ds, 40).toLower();
        m_ds >> rec.offset >> rec.fullSize;
        rec.compressedSize = 0;
    }
    bool result = true;

    std::vector<char> buffer;
    for (const auto& rec : records) {
        m_ds.device()->seek(rec.offset);
        const std::string resourceId = rec.filename.toStdString();
        std::string       fullIdStr  = resourceId;
        std::string       shortIdStr = fullIdStr;

        const std_path resourceFilenameOut = QString2stdPath(rec.filename + ".wav");
        const std_path resourceFilenameTmp = QString2stdPath("tmp_" + rec.filename + ".wav");
        std::string    subFolder           = "Effects";
        std_path       destPath            = task.destResourceRoot / subFolder;
        std_path       resourcePathOut     = destPath / resourceFilenameOut;
        const std_path resourcePathTmp     = task.destResourceRoot / resourceFilenameTmp;
        const auto*    knownResource       = m_knownResources.find(fullIdStr);
        if (knownResource) {
            subFolder       = knownResource->destinationSubfolder;
            destPath        = task.destResourceRoot / subFolder;
            resourcePathOut = destPath / resourceFilenameOut;
            fullIdStr       = knownResource->newId;
            shortIdStr      = knownResource->newId;
            if (needSkipResource(task, knownResource->newId, type))
                continue;
        }

        if (needSkipResource(task, QString2stdPath(rec.filename), type))
            continue;

        if (estimate) {
            (*estimate)++;
            continue;
        }
        m_extractCallbackInc();

        std::ofstream ofs(resourcePathTmp, std::ios::binary | std::ios::trunc);
        if (!ofs)
            return false;

        buffer.resize(rec.fullSize);
        m_ds.readRawData(buffer.data(), rec.fullSize);
        ofs.write(buffer.data(), rec.fullSize);

        ofs.close();

        if (!std_fs::exists(resourcePathOut.parent_path()))
            std_fs::create_directories(resourcePathOut.parent_path());

        auto conversionRoutine = [keepTmp = m_keepTmp, resourcePathTmp, resourcePathOut] {
            MediaConverter conv;
            if (!conv.prepareWav(resourcePathTmp, resourcePathOut))
                return;
            if (!keepTmp)
                std_fs::remove(resourcePathTmp);
        };
        conversion(conversionRoutine);

        //task.resources->registerResource({ type, fullIdStr, subFolder + "/", path2string(resourceFilenameOut), {} });
    }
    return result;
}

bool ArchiveParser::extractVID(const ExtractionTask& task, CallbackInserter& conversion, int* estimate)
{
    if (!m_requiredTypes.contains(ResourceType::Video))
        return false;

    uint32_t recordsCount;
    m_ds >> recordsCount;

    std::set<int> offsets;

    QVector<Record> records(recordsCount);

    for (auto& rec : records) {
        rec.filename = readString(m_ds, 40).toLower();
        m_ds >> rec.offset;
        rec.compressedSize = 0;
        rec.fullSize       = 0;
        offsets.insert(rec.offset);
    }
    offsets.insert(m_ds.device()->size());

    const std_path destRoot = task.destResourceRoot / "Video";
    ensureDirExistence(destRoot);

    std::vector<char> buffer;
    for (const auto& rec : records) {
        m_ds.device()->seek(rec.offset);
        const std_path resourceFilename = QString2stdPath(rec.filename);
        const std_path destFilename     = std_path(resourceFilename).replace_extension(".webp");
        const std_path resourcePath     = destRoot / resourceFilename;
        const std_path destPath         = destRoot / destFilename;
        const std_path resourceId       = resourceFilename.stem();

        auto it = offsets.find(rec.offset);
        it++;
        const auto fullSize = *it - rec.offset;

        if (needSkipResource(task, resourceId, ResourceType::Video))
            continue;

        if (estimate) {
            (*estimate)++;
            continue;
        }
        m_extractCallbackInc();

        std::ofstream ofs(resourcePath, std::ios::binary | std::ios::trunc);
        if (!ofs)
            return false;

        buffer.resize(fullSize);
        m_ds.readRawData(buffer.data(), fullSize);
        ofs.write(buffer.data(), fullSize);

        ofs.close();
        auto conversionRoutine = [keepTmp = m_keepTmp, resourcePath, destPath] {
            MediaConverter conv;
            if (!conv.prepareVideo(resourcePath, destPath))
                return;
            if (!keepTmp)
                std_fs::remove(resourcePath);
        };

        conversion(conversionRoutine);

        //task.resources->registerResource({ ResourceType::Video, path2string(resourceId), "Video/", path2string(destFilename), {} });
    }
    return true;
}

bool ArchiveParser::needSkipResource(const ExtractionTask& task, const std_path& resourceId, const std_path& resourceExt)
{
    const ResourceType type = guessType(resourceExt, nullptr);
    return needSkipResource(task, resourceId, type);
}

bool ArchiveParser::needSkipResource(const ExtractionTask& task, const std_path& resourceId, ResourceType type)
{
    if (!m_requiredTypes.contains(type))
        return true;

    if (!m_overrideExisting) {
        const bool alreadyExists = task.resources->fileExists(type, path2string(resourceId));
        return alreadyExists;
    }

    return false;
}

}
