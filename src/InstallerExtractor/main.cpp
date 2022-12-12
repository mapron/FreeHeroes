/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include <Windows.h>
#include <io.h>
#include <cstdio>

#include <cstdlib>
#include <cstring>
#include <tchar.h>

#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <list>
#include <algorithm>
#include <memory>
#include <fstream>
#include <cwchar>
#include <functional>

#include "TmpDir.hpp"
#include "TmpArchiveResource.hpp"

#include "SplashWindow.hpp"
#include "ProcessUtils.hpp"

namespace {

const std::wstring g_extractorFileMarker = L"need_cleanup.txt";
#ifdef NDEBUG
const std::wstring g_mainApp{ L"PostInstallSetup.exe" };
#else
const std::wstring g_mainApp{ L"PostInstallSetupD.exe" };
#endif

DWORD WINAPI ExtractThreadFunction(LPVOID lpParam)
{
    auto* callback = reinterpret_cast<std::function<void()>*>(lpParam);
    (*callback)();
    return 0;
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE /*hPrevInstance*/,
                   LPSTR /*lpCmdLine*/,
                   int /*nCmdShow*/)
{
    DWORD exitCode;
    try {
        const std::wstring cmdLine = ::GetCommandLine();

        // 1. create temp dir in AppData
        TmpDir tmpPath;
        std::wcout << "tmpPath=" << tmpPath.path() << std::endl;

        SplashWindow splash;

        // 2. Load slash from exe resource
        splash.LoadSplashBitmap();

        // 3. Show splash bitmap as HWND
        splash.CreateNativeWindow(hInstance);

        splash.Show();

        // 4. Extract 7z resources

        std::function<void()> extractCallback([&tmpPath]() {
            try {
                TmpArchiveResource tmpArchive;
                tmpArchive.ExtractFiles(tmpPath.path());
            }
            catch (std::exception& ex) {
                MessageBoxA(nullptr, ex.what(), "Installer error", MB_OK | MB_ICONERROR);
            }
        });

        HANDLE threadHandle = CreateThread(
            nullptr,               // default security attributes
            0,                     // use default stack size
            ExtractThreadFunction, // thread function name
            &extractCallback,      // argument to thread function
            0,                     // use default creation flags
            nullptr);              // returns the thread identifier

        {
            while (true) {
                if (WaitForSingleObject(threadHandle, 10) != WAIT_TIMEOUT)
                    break;
                splash.KeepResponding();
            }
            CloseHandle(threadHandle);
        }

        // 5. Create marker file to check for folder cleanup needed
        const auto markerPath = tmpPath.path() + g_extractorFileMarker;
        {
            std::ofstream markerFile;
            markerFile.open(markerPath, std::ios::out);
        }

        // 6. Launch postinstall setup application
        std::wstring appName = tmpPath.path() + g_mainApp;

        PROCESS_INFORMATION piProcInfo;
        BOOL                resEx = CreateProcessWrapper(piProcInfo,
                                          appName,
                                          L"",
                                          tmpPath.path(),
                                          /*noWindow=*/false,
                                          /*inheritConsole=*/true);

        splash.Hide();

        if (!resEx)
            throw std::runtime_error("Failed to start installer front-end application.");

        // 7. Wait until postinstall closed.
        WaitProcessWrapper(piProcInfo, exitCode);

        if (_waccess(markerPath.c_str(), 0) == -1) {
            // cleanup file was deleted = keep directory
            tmpPath.keep();
            std::wcout << "Keeping tmpPath." << std::endl;
        } else {
            std::wcout << "Cleaning tmpPath." << std::endl;
        }
    }
    catch (std::exception& ex) {
        MessageBoxA(nullptr, ex.what(), "Installer error", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
    return exitCode;
}
