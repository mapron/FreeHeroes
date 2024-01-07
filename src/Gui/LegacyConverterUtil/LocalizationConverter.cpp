/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#ifndef DISABLE_QT
#include "LocalizationConverter.hpp"

#include "FsUtilsQt.hpp"
#include "IGameDatabase.hpp"
#include "MernelPlatform/Logger.hpp"

#include "MernelPlatform/FileFormatJson.hpp"
#include "MernelPlatform/FileIOUtils.hpp"
#include "MernelPlatform/PropertyTree.hpp"
#include "MernelPlatform/StringUtils.hpp"

#include <QFile>
#include <QTextCodec>

#include <fstream>

namespace FreeHeroes {
using namespace Core;
using namespace Mernel;

namespace {
constexpr const std::string_view g_hdatChapterSeparator{ "\r\n=============================\r\n" };
}

class LocalizationConverter::TranscodedFile {
    QTextCodec*          m_inputCodec = nullptr;
    std::string          m_localeId;
    Mernel::PropertyTree m_data;

    Mernel::PropertyTreeMap* m_records = nullptr;
    const std_path           m_outJsonFilename;

public:
    TranscodedFile(const std_path& outJsonFilename, QByteArray encoding)
        : m_inputCodec{ QTextCodec::codecForName(encoding) }
        , m_outJsonFilename(outJsonFilename)
    {
        m_data.convertToList();
    }
    std::string toString(const QByteArray& value)
    {
        auto valueStr = m_inputCodec->toUnicode(value);
        valueStr.replace("\r\n", "\\n");
        valueStr.replace('\n', "\\n");
        return valueStr.toStdString();
    }
    void setLocale(const std::string& localeId) { this->m_localeId = localeId; }
    void setContext(const std::string& contextId)
    {
        Mernel::PropertyTree segment;
        segment["scope"] = PropertyTreeScalar(contextId);
        segment["records"].convertToMap();
        m_data.append(segment);
        m_records = &(m_data.getList().back()["records"].getMap());
    }
    void writeRow(const std::string& objectKey, const std::string& tsKey, const QByteArray& value)
    {
        Mernel::PropertyTree& rec      = (*m_records)[objectKey];
        Mernel::PropertyTree& recPres  = rec["pres"];
        Mernel::PropertyTree& presName = recPres[tsKey];
        Mernel::PropertyTree& ts       = presName["ts"];
        Mernel::PropertyTree& tsLoc    = ts[m_localeId];
        tsLoc                          = PropertyTreeScalar(toString(value));
    }

    void writeChildRow(const std::string& objectKey, const std::string& childKey, const std::string& tsKey, const QByteArray& value)
    {
        Mernel::PropertyTree& rec      = (*m_records)[objectKey];
        Mernel::PropertyTree& recPres  = rec[childKey]["pres"];
        Mernel::PropertyTree& presName = recPres[tsKey];
        Mernel::PropertyTree& ts       = presName["ts"];
        Mernel::PropertyTree& tsLoc    = ts[m_localeId];
        tsLoc                          = PropertyTreeScalar(toString(value));
    }

    void finish()
    {
        std::string buffer = Mernel::writeJsonToBuffer(m_data, false);
        Mernel::writeFileFromBuffer(m_outJsonFilename, buffer);
    }
};

LocalizationConverter::LocalizationConverter(const Core::IGameDatabase* databaseHOTA,
                                             const Core::IGameDatabase* databaseSOD)

{
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

void LocalizationConverter::extractSOD(const std_path& txtSubdir, const std_path& outJsonFilename)
{
    TxtTable crtraits = readTable(txtSubdir / "crtraits.txt");
    if (crtraits.isEmpty())
        return;

    QString     localeId         = "en_US";
    QByteArray  encoding         = "Latin-1";
    const auto& pikemanLocalized = crtraits[2][0];
    // encoding and locale guess based on "Pikeman" translation.
    if (pikemanLocalized == "Pikeman") {
    } else if (pikemanLocalized.startsWith("\xCA\xEE\xEF\xE5")) { // "Копе", Russian, cp1251 symbols represented as escape sequence.
        localeId = "ru_RU";
        encoding = "Windows-1251";
    } else if (pikemanLocalized == "Pikenier") {
        localeId = "de_DE";
        encoding = "Windows-1252";
    } else if (pikemanLocalized.startsWith("\xC7\xB9\xB1\xF8")) { // Chinese
        localeId = "zh_CN";
        encoding = "GB18030";
    }
    //units.create.en_US
    //<subjectName>.<localeId>

    TranscodedFile outFile(outJsonFilename, encoding);
    outFile.setLocale(localeId.toStdString());

    const size_t tableOffset = 2;
    assert(crtraits.size() - tableOffset == m_idSetSOD.unitIds.size());
    outFile.setContext("units");
    for (size_t index = tableOffset; index < (size_t) crtraits.size(); index++) {
        auto& id  = m_idSetSOD.unitIds[index - tableOffset];
        auto& row = crtraits[index];
        if (id.empty())
            continue;

        outFile.writeRow(id, "name", row[0]);
        if (row.size() >= 2 && !row[1].isEmpty())
            outFile.writeRow(id, "namePlural", row[1]);
    }

    {
        outFile.setContext("artifacts");
        TxtTable artraits = readTable(txtSubdir / "artraits.txt");
        for (size_t index = 0; index < m_idSetSOD.artifactIds.size(); index++) {
            auto& row   = artraits[tableOffset + index];
            auto& id    = m_idSetSOD.artifactIds[index];
            auto  descr = row[row.size() - 1];

            auto& name = row[0];
            outFile.writeRow(id, "name", name);
            outFile.writeRow(id, "descr", descr);
        }
    }
    {
        outFile.setContext("heroes");
        TxtTable hotraits = readTable(txtSubdir / "hotraits.txt");
        TxtTable herobios = readTable(txtSubdir / "herobios.txt");
        for (size_t index = 0; index < m_idSetSOD.heroesIds.size(); index++) {
            auto& rowHero     = hotraits[tableOffset + index];
            auto& rowHeroBios = herobios[index];
            auto& id          = m_idSetSOD.heroesIds[index];
            auto& name        = rowHero[0];
            auto& bio         = rowHeroBios[0];

            outFile.writeRow(id, "name", name);
            outFile.writeRow(id, "bio", bio);
        }
    }
    {
        outFile.setContext("skills");
        TxtTable sstraits = readTable(txtSubdir / "sstraits.txt");
        for (size_t index = 0; index < m_idSetSOD.skillsIds.size(); index++) {
            auto& row = sstraits[tableOffset + index];
            auto& id  = m_idSetSOD.skillsIds[index];

            outFile.writeRow(id, "name", row[0]);
            outFile.writeRow(id, "descrBasic", row[1]);
            outFile.writeRow(id, "descrAdvanced", row[2]);
            outFile.writeRow(id, "descrExpert", row[3]);
        }
    }
    {
        outFile.setContext("spells");
        TxtTable sptraits1 = readTable(txtSubdir / "sptraits.txt");
        TxtTable sptraits;
        for (auto row : sptraits1) {
            if (!row[1].isEmpty())
                sptraits << row;
        }
        for (size_t index = 0; index < m_idSetSOD.spellIds.size(); index++) {
            auto&  row            = sptraits[1 + index];
            auto&  id             = m_idSetSOD.spellIds[index];
            size_t shortDescIndex = row.size() - 5;
            outFile.writeRow(id, "name", row[0]);
            outFile.writeRow(id, "shortName", row[1]);
            if (row[shortDescIndex + 0].isEmpty())
                continue;

            outFile.writeRow(id, "descrNormal", row[shortDescIndex + 0]);
            outFile.writeRow(id, "descrBasic", row[shortDescIndex + 1]);
            outFile.writeRow(id, "descrAdvanced", row[shortDescIndex + 2]);
            outFile.writeRow(id, "descrExpert", row[shortDescIndex + 3]);
        }
    }
    {
        outFile.setContext("factions");
        TxtTable hctraits = readTable(txtSubdir / "hctraits.txt");
        for (size_t index = 0; index < m_idSetSOD.factionIds.size(); index++) {
            auto& id = m_idSetSOD.factionIds[index];
            {
                auto& row = hctraits[tableOffset + index * 2 + 0];
                outFile.writeChildRow(id, "warriorClass", "name", row[0]);
            }
            {
                auto& row = hctraits[tableOffset + index * 2 + 1];
                outFile.writeChildRow(id, "mageClass", "name", row[0]);
            }
        }
    }
    outFile.finish();
}

void LocalizationConverter::extractHOTA(const std_path& jsonSubdir, const std_path& outJsonFilename)
{
    TxtTable locTable;
    auto     loadJsonRow = [&jsonSubdir](const std::string& prefix, size_t index) -> std::vector<std::string> {
        std::vector<std::string> res;
        auto                     filename = jsonSubdir / (prefix + std::to_string(index) + ".txt");

        std::string chaptersStr;
        if (!Mernel::readFileIntoBufferNoexcept(filename, chaptersStr))
            return {};

        res = Mernel::splitLine(chaptersStr, std::string(g_hdatChapterSeparator));
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
    std::string localeId = "en_US";
    QByteArray  encoding = "utf-8";
    // encoding and locale guess based on "Cannon" translation.
    const auto& cannonLocalized = locTable[0][1];
    if (cannonLocalized == "Cannon") {
    } else if (cannonLocalized.startsWith("\xcf\xf3\xf8\xea\xe0")) { // "Пушка", Russian, cp1251 symbols represented as escape sequence.
        localeId = "ru_RU";
        encoding = "Windows-1251";
    } else if (cannonLocalized.startsWith("Kanone")) { // German
        localeId = "de_DE";
        encoding = "Windows-1252";
    } else if (cannonLocalized.startsWith("\xbc\xd3\xc5\xa9\xc5\xda")) { // Chinese
        localeId = "zh_CN";
        encoding = "GB18030";
    }

    Logger(Logger::Notice) << "localeId=" << localeId << ", encoding=" << encoding.toStdString();

    TranscodedFile outFile(outJsonFilename, encoding);
    outFile.setLocale(localeId);
    outFile.setContext("units");
    for (auto row : locTable) {
        outFile.writeRow(row[0].toStdString(), "name", row[1]);
        if (row.size() >= 3 && !row[2].isEmpty())
            outFile.writeRow(row[0].toStdString(), "namePlural", row[1]);
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
            outFile.writeRow(id, "name", QByteArray::fromStdString(name));
            outFile.writeRow(id, "descr", QByteArray::fromStdString(descr));
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
            outFile.writeRow(id, "name", QByteArray::fromStdString(name));
            outFile.writeRow(id, "bio", QByteArray::fromStdString(bio));
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

            outFile.writeRow(id, "name", QByteArray::fromStdString(name));

            outFile.writeRow(id, "descrBasic", basic);
            outFile.writeRow(id, "descrAdvanced", advan);
            outFile.writeRow(id, "descrExpert", exper);
        }
    }
    {
        outFile.setContext("factions");
        for (size_t i = m_idSetSOD.factionIds.size(); i < m_idSetHOTA.factionIds.size(); ++i) {
            auto& id = m_idSetHOTA.factionIds[i];

            {
                auto jsonRow = loadJsonRow("class", i * 2 + 0);
                outFile.writeChildRow(id, "warriorClass", "name", QByteArray::fromStdString(jsonRow[1]));
            }
            {
                auto jsonRow = loadJsonRow("class", i * 2 + 1);
                outFile.writeChildRow(id, "mageClass", "name", QByteArray::fromStdString(jsonRow[1]));
            }
        }
    }
    outFile.finish();
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
#endif
