/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LocalizationConverter.hpp"
#include "LegacyMapping.hpp"

#include "FsUtilsQt.hpp"

#include <QFile>
#include <QTextCodec>

#include <json.hpp>

#include <fstream>

using namespace nlohmann;

namespace FreeHeroes::Conversion {
using namespace Core;

class LocalizationConverter::TranscodedFile {
    //QFile outFile;
    QTextCodec* m_inputCodec = nullptr;
    std::string m_localeId;
    std::string m_contextId;
    Core::IResourceLibrary & m_resources;
public:
    TranscodedFile(Core::IResourceLibrary & resources, QByteArray encoding)
        :  m_inputCodec{QTextCodec::codecForName(encoding)}
        , m_resources(resources)
    {
      //  outFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    }
    std::string toString(const QByteArray & value) {
        auto valueStr = m_inputCodec->toUnicode(value);
        valueStr.replace("\r\n", "\\n");
        valueStr.replace('\n', "\\n");
        return valueStr.toStdString();
    }
    void setLocale(const std::string & localeId) { this->m_localeId = localeId; }
    void setContext(const std::string & contextId) { this->m_contextId = contextId; }
    void writeRow (const std::string & key, const QByteArray & value) {
        m_resources.registerResource(Core::ResourceTranslation{m_localeId, m_contextId, key, {toString(value)}});
    };
    void writeRow (const std::string & key, const QByteArray & value, const QByteArray & value2) {
        m_resources.registerResource(Core::ResourceTranslation{m_localeId, m_contextId, key, {toString(value), toString(value2)}});
    };
};

LocalizationConverter::LocalizationConverter(IResourceLibrary& resources,
                                             const std_path& srcRoot)
    : m_resources(resources)
    , m_root(srcRoot)
    , m_rootDest(srcRoot / "Translation")
{
    if (!std_fs::exists(m_rootDest))
        std_fs::create_directories(m_rootDest);
}

void LocalizationConverter::extractSOD(const std_path& txtSubdir)
{
    TxtTable crtraits = readTable(m_root / txtSubdir / "crtraits.txt");
    if (crtraits.isEmpty())
        return;


    QString localeId = "";
    QByteArray encoding = "Latin-1";
    if (crtraits[2][0] == "Pikeman") {
        localeId = "en_US";
    } else if (crtraits[2][0].startsWith( "\xCA\xEE\xEF\xE5" ) ) { // "Копе" in Russian Win1251
        localeId = "ru_RU";
        encoding = "Windows-1251";
    } else {
        localeId = "en_US"; // no idea of sane fallback.
    }
    //units.create.en_US
    //<subjectName>.<localeId>
   // const std::string resourceId = "sodLoc." + localeId.toStdString();
   // const std_path mainFilename = ("translation." + localeId.toStdString() + ".txt");
   // const std_path localizationFile = m_rootDest / mainFilename;


    TranscodedFile outFile(m_resources, encoding);
    outFile.setLocale(localeId.toStdString());

    const size_t tableOffset = 2;
    assert(crtraits.size() - tableOffset == unitIds[VersionSod].size());
    outFile.setContext("units");
    for (size_t index = tableOffset; index < (size_t)crtraits.size(); index++) {
        auto & id = unitIds[VersionSod][index - tableOffset];
        auto & row = crtraits[index];
        if (id.empty())
            continue;

        outFile.writeRow(id, row[0], row[1]);
    }

    {
        outFile.setContext("artifacts");
        TxtTable artraits = readTable(m_root / txtSubdir / "artraits.txt");
        for (size_t index = 0; index < artifactIds[VersionSod].size(); index++) {
            auto & row = artraits[tableOffset + index];
            auto & id = artifactIds[VersionSod][index];
            auto descr = row[row.size()-1];

            auto & name = row[0];
            outFile.writeRow(id, name);
            outFile.writeRow(id + ".descr", descr);
        }
    }
    {
        outFile.setContext("heroes");
        TxtTable hotraits = readTable(m_root / txtSubdir / "hotraits.txt");
        TxtTable herobios = readTable(m_root / txtSubdir / "herobios.txt");
        for (size_t index = 0; index < heroesIds[VersionSod].size(); index++) {
            auto & rowHero = hotraits[tableOffset + index];
            auto & rowHeroBios = herobios[index];
            auto & id = heroesIds[VersionSod][index];
            auto & name = rowHero[0];
            auto & bio = rowHeroBios[0];
            outFile.writeRow(id, name);
            outFile.writeRow(id + ".bio", bio);
        }
    }
    {
        outFile.setContext("skills");
        TxtTable sstraits = readTable(m_root / txtSubdir / "sstraits.txt");
        for (size_t index = 0; index < skillsIds[VersionSod].size(); index++) {
            auto & row = sstraits[tableOffset + index];
            auto & id = skillsIds[VersionSod][index];

            outFile.writeRow(id              , row[0]);
            outFile.writeRow(id + ".basic"   , row[1]);
            outFile.writeRow(id + ".advanced", row[2]);
            outFile.writeRow(id + ".expert"  , row[3]);
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
        for (size_t index = 0; index < spellIds[VersionSod].size(); index++) {
            auto & row = sptraits[1 + index];
            auto & id = spellIds[VersionSod][index];
            size_t shortDescIndex = row.size() - 5;
            outFile.writeRow(id,              row[0]);
            outFile.writeRow(id + ".short"  , row[1]);
            if (row[shortDescIndex + 0].isEmpty())
                continue;

            outFile.writeRow(id + ".normal"  , row[shortDescIndex + 0]);
            outFile.writeRow(id + ".basic"   , row[shortDescIndex + 1]);
            outFile.writeRow(id + ".advanced", row[shortDescIndex + 2]);
            outFile.writeRow(id + ".expert"  , row[shortDescIndex + 3]);
        }
    }
    {
        outFile.setContext("classes");
        TxtTable hctraits = readTable(m_root / txtSubdir / "hctraits.txt");
        for (size_t index = 0; index < classIds[VersionSod].size(); index++) {
            auto & row = hctraits[tableOffset + index];
            auto & id = classIds[VersionSod][index];

            outFile.writeRow(id, row[0]);
        }
    }

  //  m_resources.registerResource(ResourceTranslation{resourceId, path2string(m_rootDest), path2string(mainFilename) });
}

void LocalizationConverter::extractHOTA(const std_path& jsonSubdir)
{
    TxtTable locTable;
    auto loadJsonRow = [this, &jsonSubdir](const std::string & prefix, size_t index) -> std::vector<std::string>{
        std::vector<std::string> res;
        auto filename = m_root / jsonSubdir / (prefix + std::to_string(index) + ".json");

        std::ifstream monJson(filename);
        if (!monJson)
            return res;

        json monJsonObj;
        monJson >> monJsonObj;
        auto & strings = monJsonObj["strings"];
        for (size_t i = 0; i< strings.size(); ++i)
            res.push_back(static_cast<std::string>(strings[i]));
       return res;
    };

    for (size_t i = 0, i2 = unitIds[VersionSod].size(); i < unitIds[VersionHota].size(); ++i, ++i2) {
        auto & id = unitIds[VersionHota][i];
        if (id.empty())
            continue;
        auto jsonRow = loadJsonRow("monst", i2);
        if (jsonRow.empty())
            continue;

        const std::string name1 = jsonRow[3];
        const std::string name2 = jsonRow[4];
        locTable.push_back({QByteArray::fromStdString(id), QByteArray::fromStdString(name1), QByteArray::fromStdString(name2)});
    }
    std::string localeId = "en_US";
    const QByteArray encoding = "utf-8";
    if (locTable[0][1].startsWith("\xD0\x9F\xD1\x83\xD1\x88")) // "Пуш", Russian, utf-8
        localeId = "ru_RU";

    TranscodedFile outFile(m_resources, encoding);
    outFile.setLocale(localeId);
    outFile.setContext("units");
    for (auto row : locTable) {
        outFile.writeRow(row[0].toStdString(), row[1], row[2]);
    }


    {
        outFile.setContext("artifacts");
        for (size_t i = 0, i2 = artifactIds[VersionSod].size(); i < artifactIds[VersionHota].size(); ++i, ++i2) {
            auto & id = artifactIds[VersionHota][i];
            if (id.empty())
                continue;
            auto jsonRow = loadJsonRow("art", i2);
            if (jsonRow.empty())
                continue;
            const std::string name = jsonRow[1];
            const std::string onGetDescr = jsonRow[2];
            const std::string descr = jsonRow[7];
           outFile.writeRow(id, QByteArray::fromStdString(name));
           outFile.writeRow(id + ".descr", QByteArray::fromStdString(descr));
        }
    }
    {
        outFile.setContext("heroes");
        for (size_t i = 0, i2 = heroesIds[VersionSod].size(); i < heroesIds[VersionHota].size(); ++i, ++i2) {
            auto & id = heroesIds[VersionHota][i];
            if (id.empty())
                continue;
            auto jsonRow = loadJsonRow("hero", i2);
            if (jsonRow.empty())
                continue;
            const std::string name = jsonRow[6];
            const std::string bio = jsonRow[7];
            outFile.writeRow(id, QByteArray::fromStdString(name));
            outFile.writeRow(id + ".bio", QByteArray::fromStdString(bio));
        }
    }
    {
        outFile.setContext("skills");
        for (size_t i = 0, i2 = skillsIds[VersionSod].size(); i < skillsIds[VersionHota].size(); ++i, ++i2) {
            auto & id = skillsIds[VersionHota][i];
            if (id.empty())
                continue;
            auto jsonRow = loadJsonRow("secskill", i2);
            if (jsonRow.empty())
                continue;
            const std::string name = jsonRow[1];
            const std::string descr = jsonRow[7];
            auto parts = QByteArray::fromStdString(descr).split('#');
            auto basic   = parts[1].mid(5); basic=basic.mid(0, basic.size()-3);
            auto advan   = parts[2].mid(5); advan=advan.mid(0, advan.size()-3);
            auto exper   = parts[3].mid(5); exper=exper.mid(0, exper.size()-3);

            outFile.writeRow(id, QByteArray::fromStdString(name));
            outFile.writeRow(id + ".basic"   , basic);
            outFile.writeRow(id + ".advanced", advan);
            outFile.writeRow(id + ".expert"  , exper);
        }
    }
    {
        outFile.setContext("classes");
        for (size_t i = 0, i2 = classIds[VersionSod].size(); i < classIds[VersionHota].size(); ++i, ++i2) {
            auto & id = classIds[VersionHota][i];
            if (id.empty())
                continue;
            auto jsonRow = loadJsonRow("class", i2);
            if (jsonRow.empty())
                continue;
            const std::string name = jsonRow[1];
            outFile.writeRow(id, QByteArray::fromStdString(name));
        }
    }

}

LocalizationConverter::TxtTable LocalizationConverter::readTable(const LocalizationConverter::std_path& filename) const
{
    QFile file(Gui::stdPath2QString(filename));
    if (!file.open(QIODevice::ReadOnly))
        return {};

   TxtTable result;
   QByteArray data = file.readAll();
   int offset = 0, ind = 0;
   while((ind = data.indexOf("\r\n", offset)) != -1) { // sadly can't split by string. and we cant use QString too.
       QByteArray rowStr = data.mid(offset, ind - offset);
       offset = ind + 2;

       TxtRow row = rowStr.split('\t');
       bool isEmpty = true;
       for (auto & rowCell : row)
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
