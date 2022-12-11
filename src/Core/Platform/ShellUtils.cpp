/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "ShellUtils.hpp"

#ifdef _WIN32
#include "ScopeExit.hpp"

#include <Objbase.h>
#include <Shlobj.h>

namespace FreeHeroes::Core {

namespace {
class CoInitializer {
public:
    CoInitializer() { ::CoInitialize(NULL); }

    ~CoInitializer() { ::CoUninitialize(); }
};

const std::string g_linkExtension = ".lnk";

}

bool showFolderInFileManager(const std_path& path)
{
    CoInitializer coInit;
    return ::ShellExecuteW(nullptr, nullptr, path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL) > HINSTANCE(32);
}

bool createShortCut(const std_path&    fromDir,
                    const std::string& fromName,
                    const std_path&    toDir,
                    const std::string& toFilename,
                    const std::string& extraArgs)
{
    CoInitializer coInit;
    auto          linkPath = fromDir / (fromName + g_linkExtension);

    const std::wstring extraArgsW(extraArgs.cbegin(), extraArgs.cend());

    const std_path to = toDir / toFilename;

    IShellLink* psl          = nullptr;
    bool        neededCoInit = false;

    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**) &psl);

    if (hres == CO_E_NOTINITIALIZED) { // COM was not initialized
        neededCoInit = true;
        CoInitialize(NULL);
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**) &psl);
    }
    FH_SCOPE_EXIT([&psl, neededCoInit] {
        if (psl)
            psl->Release();
        if (neededCoInit)
            CoUninitialize();
    });
    if (!SUCCEEDED(hres))
        return false;
    if (!SUCCEEDED(psl->SetPath(to.wstring().c_str())))
        return false;
    if (!SUCCEEDED(psl->SetWorkingDirectory(toDir.wstring().c_str())))
        return false;

    if (!extraArgs.empty() && !SUCCEEDED(psl->SetArguments(extraArgsW.c_str())))
        return false;

    IPersistFile* ppf = nullptr;
    hres              = psl->QueryInterface(IID_IPersistFile, (void**) &ppf);
    FH_SCOPE_EXIT([&ppf] {
        if (ppf)
            ppf->Release();
    });
    if (!SUCCEEDED(hres))
        return false;

    hres = ppf->Save(linkPath.wstring().c_str(), TRUE);
    if (!SUCCEEDED(hres)) {
        return false;
    }

    return true;
}

}
#else
namespace FreeHeroes::Core {

bool showFolderInFileManager(const std_path&)
{
    return false;
}

bool FreeHeroes::Core::createShortCut(const std_path&    fromDir,
                                      const std::string& fromName,
                                      const std_path&    toDir,
                                      const std::string& toFilename,
                                      const std::string& extraArgs)
{
    return false;
}

}
#endif
