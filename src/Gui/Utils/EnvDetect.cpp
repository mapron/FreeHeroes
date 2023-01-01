/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "EnvDetect.hpp"
#include "FsUtilsQt.hpp"

#include <QSettings>

#ifdef _WIN32

FreeHeroes::Core::std_path FreeHeroes::findHeroes3Installation(bool hotaAllowed) noexcept
{
    std::vector<std::pair<QString, QString>> lookup;
    if (hotaAllowed) {
        lookup.push_back({ R"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\HotA + HD_is1)", "InstallLocation" });
    }
    lookup.push_back({ R"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\1207658787_is1)", "InstallLocation" });
    lookup.push_back({ R"(HKEY_LOCAL_MACHINE\SOFTWARE\GOG.com\Games\1207658787)", "path" });

    for (const auto& [branch, key] : lookup) {
        QSettings reg(branch, QSettings::Registry32Format);
        auto      path = Gui::QString2stdPath(reg.value(key).toString());
        if (path.empty())
            continue;
        std::error_code ec;
        if (!Core::std_fs::exists(path, ec))
            continue;

        if (!Core::std_fs::exists(path / "Data", ec) && !Core::std_fs::exists(path / "DATA", ec) && !Core::std_fs::exists(path / "data", ec))
            continue;

        return path;
    }

    return {};
}

#else

FreeHeroes::Core::std_path FreeHeroes::findHeroes3Installation(bool)
{
    return {};
}

#endif
