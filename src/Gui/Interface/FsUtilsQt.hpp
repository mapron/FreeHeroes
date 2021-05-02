/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtils.hpp"

#include <QString>

namespace FreeHeroes::Gui {

namespace std_fs = Core::std_fs;
using std_path   = Core::std_path;

inline QString stdPath2QString(const std_path& path)
{
    return QString::fromStdString(Core::path2string(path));
}

inline std_path QString2stdPath(const QString& path)
{
    return Core::string2path(path.toStdString());
}

}
