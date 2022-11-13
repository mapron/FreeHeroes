/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "AppLocations.hpp"

#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <Windows.h>
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace FreeHeroes::Core {

namespace {
std_path getHomePath()
{
    char* envPath;
    char* envDrive;
    if ((envPath = getenv("USERPROFILE"))) {
        return envPath;
    } else if ((envDrive = getenv("HOMEDRIVE")) && (envPath = getenv("HOMEPATH"))) {
        return std::string(envDrive) + envPath;
    }
    if ((envPath = getenv("HOME"))) {
        return envPath;
    }
    return {};
}

std_path getAppDataDir(const std::string& orgName)
{
#ifndef _WIN32
    const std_path appDataRoot = getHomePath() / ("." + orgName);
#else
    const std_path appDataRoot = std_path(getenv("LOCALAPPDATA")) / (orgName);
#endif
    return appDataRoot;
}

std_path getExecutablePath()
{
#ifdef __APPLE__
    char     path[4096], actualpath[4096];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0)
        return path;
    char* abspath = realpath(path, actualpath);
    if (abspath)
        return std_path(abspath);
    return "./";
#elif !defined(_WIN32)
    int            len;
    char           path[1024];

    // Read symbolic link /proc/self/exe

    len = readlink("/proc/self/exe", path, sizeof(path));
    if (len == -1)
        return std::string("./unknown");

    path[len] = '\0';
    returnstd_path(path);
#else
    WCHAR szFileName[MAX_PATH];

    GetModuleFileNameW(nullptr, szFileName, MAX_PATH);
    return std_path(szFileName);
#endif
}
};

AppLocations::AppLocations(std::string orgName, std::string appName)
    : m_orgName(std::move(orgName))
    , m_appName(std::move(appName))
{
}

std_path AppLocations::getTempDir() const
{
    if (!m_tempDir.empty())
        return m_tempDir;
#ifdef _WIN32
    char buf[MAX_PATH + 1];
    int  size = GetTempPathA(MAX_PATH, buf);
    m_tempDir = std::string(buf, size - 1);
#elif defined(__APPLE__)
    m_tempDir = getenv("TMPDIR");
#else
    m_tempDir = "/tmp";
#endif
    m_tempDir /= m_orgName;
    if (!m_appName.empty())
        m_tempDir /= m_appName;

    std::error_code code;
    std_fs::create_directories(m_appName, code);

    return m_tempDir;
}

std_path AppLocations::getBinDir() const
{
    if (!m_exePath.empty())
        return m_exePath.parent_path();

    m_exePath = getExecutablePath();
    return m_exePath.parent_path();
}

std_path AppLocations::getHomeDir() const
{
    if (!m_homeDir.empty())
        return m_homeDir;

    m_homeDir = getHomePath();
    return m_homeDir;
}

std_path AppLocations::getAppdataDir() const
{
    if (!m_appdataDir.empty())
        return m_appdataDir;

    m_appdataDir = getAppDataDir(m_orgName);
    std::error_code code;
    std_fs::create_directories(m_appdataDir, code);

    return m_appdataDir;
}

}
