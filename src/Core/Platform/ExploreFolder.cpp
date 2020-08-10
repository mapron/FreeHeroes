/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ExploreFolder.hpp"

#ifdef _WIN32
#include <Objbase.h>
#include <Shlobj.h>

namespace FreeHeroes::Core {
class CoInitializer
{
public:
    CoInitializer(){::CoInitialize(NULL);}

    ~CoInitializer(){::CoUninitialize(); }
};

bool showFolderInFileManager(const std_path& path)
{
    CoInitializer coInit;
    return ::ShellExecuteW(nullptr, nullptr, path.wstring().c_str() , nullptr, nullptr, SW_SHOWNORMAL) > HINSTANCE(32);
}
}
#else
namespace FreeHeroes::Core {

bool showFolderInFileManager(const std_path& )
{
    return false;
}


}
#endif

