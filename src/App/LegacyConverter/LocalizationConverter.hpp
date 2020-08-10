/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtils.hpp"
#include "IResourceLibrary.hpp"

#include <QList>
#include <QByteArray>

namespace FreeHeroes::Conversion {

class LocalizationConverter
{
public:
    using std_path = Core::std_path;

    LocalizationConverter(Core::IResourceLibrary & resources,
                          const std_path& srcRoot);

    void extractSOD(const std_path& txtSubdir);
    void extractHOTA(const std_path& jsonSubdir);

private:
    using TxtRow = QList<QByteArray>;
    using TxtTable = QList<TxtRow>;
    class TranscodedFile;
    TxtTable readTable(const std_path & filename) const;

    Core::IResourceLibrary & m_resources;
    const std_path m_root;
    const std_path m_rootDest;
    TxtTable m_outLocalization;
};

}
