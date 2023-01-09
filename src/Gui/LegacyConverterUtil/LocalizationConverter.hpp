/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/FsUtils.hpp"
#include "IResourceLibrary.hpp"

#include "LegacyConverterUtilExport.hpp"

#include <QList>
#include <QByteArray>

namespace FreeHeroes::Core {
class IGameDatabase;
}
namespace FreeHeroes {

class LEGACYCONVERTERUTIL_EXPORT LocalizationConverter {
public:
    using std_path = Mernel::std_path;

    LocalizationConverter(const Core::IGameDatabase* databaseHOTA,
                          const Core::IGameDatabase* databaseSOD);

    void extractSOD(const std_path& txtSubdir, const std_path& outJsonFilename);
    void extractHOTA(const std_path& jsonSubdir, const std_path& outJsonFilename);

private:
    using TxtRow   = QList<QByteArray>;
    using TxtTable = QList<TxtRow>;
    class TranscodedFile;
    TxtTable readTable(const std_path& filename) const;

    struct IdSet {
        std::vector<std::string> unitIds;
        std::vector<std::string> artifactIds;
        std::vector<std::string> heroesIds;
        std::vector<std::string> skillsIds;
        std::vector<std::string> spellIds;
        std::vector<std::string> factionIds;

    } m_idSetHOTA, m_idSetSOD;
};

}
