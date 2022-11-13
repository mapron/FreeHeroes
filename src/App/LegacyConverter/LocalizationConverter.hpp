/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtils.hpp"
#include "IResourceLibrary.hpp"

#include <QList>
#include <QByteArray>

namespace FreeHeroes::Core {
class IGameDatabase;
}
namespace FreeHeroes::Conversion {

class LocalizationConverter {
public:
    using std_path = Core::std_path;

    LocalizationConverter(Core::IResourceLibrary&    resources,
                          const std_path&            srcRoot,
                          const Core::IGameDatabase* databaseHOTA,
                          const Core::IGameDatabase* databaseSOD);

    void extractSOD(const std_path& txtSubdir);
    void extractHOTA(const std_path& jsonSubdir);

private:
    using TxtRow   = QList<QByteArray>;
    using TxtTable = QList<TxtRow>;
    class TranscodedFile;
    TxtTable readTable(const std_path& filename) const;

    Core::IResourceLibrary& m_resources;
    const std_path          m_root;
    const std_path          m_rootDest;
    TxtTable                m_outLocalization;

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
