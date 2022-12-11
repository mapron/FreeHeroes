/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <Windows.h>

#include <string>
#include <utility>
#include <memory>
#include <stdexcept>

inline BOOL CreateProcessWrapper(PROCESS_INFORMATION& piProcInfo,
                                 const std::wstring&  appName,
                                 const std::wstring&  argsString,
                                 const std::wstring&  directory,
                                 bool                 noWindow       = false,
                                 bool                 inheritConsole = false)
{
    STARTUPINFO siStartInfo;

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));

    siStartInfo.cb = sizeof(STARTUPINFO);

    auto                       cmdString = appName + L' ' + argsString;
    std::unique_ptr<wchar_t[]> cmd       = std::make_unique<wchar_t[]>(cmdString.size() + 1);
    wcscpy(cmd.get(), cmdString.c_str());

    if (inheritConsole) {
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
        siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        siStartInfo.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
    }

    BOOL resEx = CreateProcess(
        appName.c_str(),                 // lpApplicationName
        cmd.get(),                       // lpCommandLine
        NULL,                            // lpProcessAttributes
        NULL,                            // lpThreadAttributes
        inheritConsole,                  // bInheritHandles
        noWindow ? CREATE_NO_WINDOW : 0, // dwCreationFlags
        NULL,                            // lpEnvironment
        directory.c_str(),               // lpCurrentDirectory
        &siStartInfo,                    // lpStartupInfo
        &piProcInfo);                    // lpProcessInformation
    return resEx;
}
inline void WaitProcessWrapper(PROCESS_INFORMATION& piProcInfo, DWORD& exitCode)
{
    auto waitResult = WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    GetExitCodeProcess(piProcInfo.hProcess, &exitCode);
    CloseHandle(piProcInfo.hProcess);
    if (waitResult != WAIT_OBJECT_0) {
        throw std::runtime_error("Waiting for the calling application is failed. Something must have gone terribly wrong.");
    }
}
