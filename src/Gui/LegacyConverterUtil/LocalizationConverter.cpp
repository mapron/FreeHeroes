/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LocalizationConverter.hpp"

#include "FsUtilsQt.hpp"
#include "IGameDatabase.hpp"
#include "MernelPlatform/Logger.hpp"

#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/FileIOUtils.hpp"

#include <QFile>
#include <QTextCodec>

#include <fstream>

namespace FreeHeroes::Conversion {
using namespace Core;
using namespace Mernel;

class LocalizationConverter::TranscodedFile {
    QTextCodec*             m_inputCodec = nullptr;
    std::string             m_localeId;
    std::string             m_contextId;
    Core::IResourceLibrary& m_resources;

public:
    TranscodedFile(Core::IResourceLibrary& resources, QByteArray encoding)
        : m_inputCodec{ QTextCodec::codecForName(encoding) }
        , m_resources(resources)
    {
        //  outFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    }
    std::string toString(const QByteArray& value)
    {
        auto valueStr = m_inputCodec->toUnicode(value);
        valueStr.replace("\r\n", "\\n");
        valueStr.replace('\n', "\\n");
        return valueStr.toStdString();
    }
    void setLocale(const std::string& localeId) { this->m_localeId = localeId; }
    void setContext(const std::string& contextId) { this->m_contextId = contextId; }
    void writeRow(const std::string& key, const QByteArray& value)
    {
        m_resources.registerResource(Core::ResourceTranslation{ m_localeId, m_contextId, key, { toString(value) } });
    };
    void writeRow(const std::string& key, const QByteArray& value, const QByteArray& value2)
    {
        m_resources.registerResource(Core::ResourceTranslation{ m_localeId, m_contextId, key, { toString(value), toString(value2) } });
    };
};

LocalizationConverter::LocalizationConverter(IResourceLibrary&          resources,
                                             const std_path&            srcRoot,
                                             const Core::IGameDatabase* databaseHOTA,
                                             const Core::IGameDatabase* databaseSOD)
    : m_resources(resources)
    , m_root(srcRoot)
    , m_rootDest(srcRoot / "Translation")
{
    if (!std_fs::exists(m_rootDest))
        std_fs::create_directories(m_rootDest);

    auto fillIds = [](IdSet* idSet, const Core::IGameDatabase* database) {
        idSet->unitIds     = database->units()->legacyOrderedIds();
        idSet->artifactIds = database->artifacts()->legacyOrderedIds();
        idSet->heroesIds   = database->heroes()->legacyOrderedIds();
        idSet->skillsIds   = database->secSkills()->legacyOrderedIds();
        idSet->spellIds    = database->spells()->legacyOrderedIds();
        idSet->factionIds  = database->factions()->legacyOrderedIds();
    };
    fillIds(&m_idSetHOTA, databaseHOTA);
    fillIds(&m_idSetSOD, databaseSOD);
}

void LocalizationConverter::extractSOD(const std_path& txtSubdir)
{
    TxtTable crtraits = readTable(m_root / txtSubdir / "crtraits.txt");
    if (crtraits.isEmpty())
        return;

    QString     localeId         = "en_US";
    QByteArray  encoding         = "Latin-1";
    const auto& pikemanLocalized = crtraits[2][0];
    if (pikemanLocalized == "Pikeman") {
    } else if (pikemanLocalized.startsWith("\xCA\xEE\xEF\xE5")) { // "Копе" in Russian Win1251
        localeId = "ru_RU";
        encoding = "Windows-1251";
    } else if (pikemanLocalized == "Pikenier") {
        localeId = "de_DE";
        encoding = "Windows-1252";
    }
    //units.create.en_US
    //<subjectName>.<localeId>

    TranscodedFile outFile(m_resources, encoding);
    outFile.setLocale(localeId.toStdString());

    const size_t tableOffset = 2;
    assert(crtraits.size() - tableOffset == m_idSetSOD.unitIds.size());
    outFile.setContext("units");
    for (size_t index = tableOffset; index < (size_t) crtraits.size(); index++) {
        auto& id  = m_idSetSOD.unitIds[index - tableOffset];
        auto& row = crtraits[index];
        if (id.empty())
            continue;

        outFile.writeRow(id, row[0], row[1]);
    }

    {
        outFile.setContext("artifacts");
        TxtTable artraits = readTable(m_root / txtSubdir / "artraits.txt");
        for (size_t index = 0; index < m_idSetSOD.artifactIds.size(); index++) {
            auto& row   = artraits[tableOffset + index];
            auto& id    = m_idSetSOD.artifactIds[index];
            auto  descr = row[row.size() - 1];

            auto& name = row[0];
            outFile.writeRow(id, name);
            outFile.writeRow(id + ".descr", descr);
        }
    }
    {
        outFile.setContext("heroes");
        TxtTable hotraits = readTable(m_root / txtSubdir / "hotraits.txt");
        TxtTable herobios = readTable(m_root / txtSubdir / "herobios.txt");
        for (size_t index = 0; index < m_idSetSOD.heroesIds.size(); index++) {
            auto& rowHero     = hotraits[tableOffset + index];
            auto& rowHeroBios = herobios[index];
            auto& id          = m_idSetSOD.heroesIds[index];
            auto& name        = rowHero[0];
            auto& bio         = rowHeroBios[0];

            outFile.writeRow(id, name);
            outFile.writeRow(id + ".bio", bio);
        }
    }
    {
        outFile.setContext("skills");
        TxtTable sstraits = readTable(m_root / txtSubdir / "sstraits.txt");
        for (size_t index = 0; index < m_idSetSOD.skillsIds.size(); index++) {
            auto& row = sstraits[tableOffset + index];
            auto& id  = m_idSetSOD.skillsIds[index];

            outFile.writeRow(id, row[0]);
            outFile.writeRow(id + ".basic", row[1]);
            outFile.writeRow(id + ".advanced", row[2]);
            outFile.writeRow(id + ".expert", row[3]);
        }
    }
    {
        outFile.setContext("spells");
        TxtTable sptraits1 = readTable(m_root / txtSubdir / "sptraits.txt");
        TxtTable sptraits;
        for (auto row : sptraits1) {
            if (!row[1].isEmpty())
                sptraits << row;
        }
        for (size_t index = 0; index < m_idSetSOD.spellIds.size(); index++) {
            auto&  row            = sptraits[1 + index];
            auto&  id             = m_idSetSOD.spellIds[index];
            size_t shortDescIndex = row.size() - 5;
            outFile.writeRow(id, row[0]);
            outFile.writeRow(id + ".short", row[1]);
            if (row[shortDescIndex + 0].isEmpty())
                continue;

            outFile.writeRow(id + ".normal", row[shortDescIndex + 0]);
            outFile.writeRow(id + ".basic", row[shortDescIndex + 1]);
            outFile.writeRow(id + ".advanced", row[shortDescIndex + 2]);
            outFile.writeRow(id + ".expert", row[shortDescIndex + 3]);
        }
    }
    {
        outFile.setContext("classes");
        TxtTable hctraits = readTable(m_root / txtSubdir / "hctraits.txt");
        for (size_t index = 0; index < m_idSetSOD.factionIds.size(); index++) {
            auto& id = m_idSetSOD.factionIds[index];
            {
                auto& row = hctraits[tableOffset + index * 2 + 0];
                outFile.writeRow(id + ".warrior", row[0]);
            }
            {
                auto& row = hctraits[tableOffset + index * 2 + 1];
                outFile.writeRow(id + ".mage", row[0]);
            }
        }
    }

    //  m_resources.registerResource(ResourceTranslation{resourceId, path2string(m_rootDest), path2string(mainFilename) });
}

void LocalizationConverter::extractHOTA(const std_path& jsonSubdir)
{
    TxtTable locTable;
    auto     loadJsonRow = [this, &jsonSubdir](const std::string& prefix, size_t index) -> std::vector<std::string> {
        std::vector<std::string> res;
        auto                     filename = m_root / jsonSubdir / (prefix + std::to_string(index) + ".json");

        std::string buffer;
        if (!Mernel::readFileIntoBuffer(filename, buffer))
            return {};

        PropertyTree root;
        if (!Mernel::readJsonFromBuffer(buffer, root))
            return {};

        const auto& strings = root["strings"].getList();
        for (const auto& str : strings)
            res.push_back(str.getScalar().toString());
        return res;
    };

    for (size_t i = m_idSetSOD.unitIds.size(); i < m_idSetHOTA.unitIds.size(); ++i) {
        auto& id = m_idSetHOTA.unitIds[i];
        if (id.empty())
            continue;
        auto jsonRow = loadJsonRow("monst", i);
        if (jsonRow.empty())
            continue;

        const std::string name1 = jsonRow[3];
        const std::string name2 = jsonRow[4];
        locTable.push_back({ QByteArray::fromStdString(id), QByteArray::fromStdString(name1), QByteArray::fromStdString(name2) });
    }
    std::string      localeId = "en_US";
    const QByteArray encoding = "utf-8";
    if (locTable[0][1].startsWith("\xD0\x9F\xD1\x83\xD1\x88")) // "Пуш", Russian, utf-8
        localeId = "ru_RU";
    else if (locTable[0][1].startsWith("Kanone")) // German
        localeId = "de_DE";

    Logger() << "localeId=" << localeId;

    TranscodedFile outFile(m_resources, encoding);
    outFile.setLocale(localeId);
    outFile.setContext("units");
    for (auto row : locTable) {
        outFile.writeRow(row[0].toStdString(), row[1], row[2]);
    }

    {
        outFile.setContext("artifacts");
        for (size_t i = m_idSetSOD.artifactIds.size(); i < m_idSetHOTA.artifactIds.size(); ++i) {
            auto& id = m_idSetHOTA.artifactIds[i];
            if (id.empty())
                continue;
            auto jsonRow = loadJsonRow("art", i);
            if (jsonRow.empty())
                continue;
            const std::string name       = jsonRow[1];
            const std::string onGetDescr = jsonRow[2];
            const std::string descr      = jsonRow[7];
            outFile.writeRow(id, QByteArray::fromStdString(name));
            outFile.writeRow(id + ".descr", QByteArray::fromStdString(descr));
        }
    }
    {
        outFile.setContext("heroes");
        for (size_t i = m_idSetSOD.heroesIds.size(); i < m_idSetHOTA.heroesIds.size(); ++i) {
            auto& id = m_idSetHOTA.heroesIds[i];
            if (id.empty())
                continue;
            auto jsonRow = loadJsonRow("hero", i);
            if (jsonRow.empty())
                continue;
            const std::string name = jsonRow[6];
            const std::string bio  = jsonRow[7];
            outFile.writeRow(id, QByteArray::fromStdString(name));
            outFile.writeRow(id + ".bio", QByteArray::fromStdString(bio));
        }
    }
    {
        outFile.setContext("skills");
        for (size_t i = m_idSetSOD.skillsIds.size(); i < m_idSetHOTA.skillsIds.size(); ++i) {
            auto& id = m_idSetHOTA.skillsIds[i];
            if (id.empty())
                continue;
            auto jsonRow = loadJsonRow("secskill", i);
            if (jsonRow.empty())
                continue;
            const std::string name  = jsonRow[1];
            const std::string descr = jsonRow[7];
            auto              parts = QByteArray::fromStdString(descr).split('#');

            auto basic = parts[1].mid(5);
            basic      = basic.mid(0, basic.size() - 3);
            auto advan = parts[2].mid(5);
            advan      = advan.mid(0, advan.size() - 3);
            auto exper = parts[3].mid(5);
            exper      = exper.mid(0, exper.size() - 3);

            outFile.writeRow(id, QByteArray::fromStdString(name));
            outFile.writeRow(id + ".basic", basic);
            outFile.writeRow(id + ".advanced", advan);
            outFile.writeRow(id + ".expert", exper);
        }
    }
    {
        outFile.setContext("classes");
        for (size_t i = m_idSetSOD.factionIds.size(); i < m_idSetHOTA.factionIds.size(); ++i) {
            auto& id = m_idSetHOTA.factionIds[i];

            {
                auto jsonRow = loadJsonRow("class", i * 2 + 0);
                outFile.writeRow(id + ".warrior", QByteArray::fromStdString(jsonRow[1]));
            }
            {
                auto jsonRow = loadJsonRow("class", i * 2 + 1);
                outFile.writeRow(id + ".mage", QByteArray::fromStdString(jsonRow[1]));
            }
        }
    }
}

LocalizationConverter::TxtTable LocalizationConverter::readTable(const LocalizationConverter::std_path& filename) const
{
    QFile file(Gui::stdPath2QString(filename));
    if (!file.open(QIODevice::ReadOnly))
        return {};

    TxtTable   result;
    QByteArray data   = file.readAll();
    int        offset = 0, ind = 0;
    while ((ind = data.indexOf("\r\n", offset)) != -1) { // sadly can't split by string. and we cant use QString too.
        QByteArray rowStr = data.mid(offset, ind - offset);
        offset            = ind + 2;

        TxtRow row     = rowStr.split('\t');
        bool   isEmpty = true;
        for (auto& rowCell : row)
            if (!rowCell.trimmed().isEmpty()) {
                isEmpty = false;
                break;
            }
        if (!isEmpty)
            result << row;
    }
    return result;
}

}
