/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TmpDir.hpp"

#include <Windows.h>

#include <stdexcept>

namespace {

BOOL DirectoryExists(LPCTSTR szPath)
{
    DWORD dwAttrib = GetFileAttributes(szPath);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

} // namespace

const std::wstring& TmpDir::path() const
{
    return m_path;
}

TmpDir::TmpDir()
{
    const std::string appData = getenv("APPDATA"); // Roaming

    const std::wstring tmpRoot(appData.cbegin(), appData.cend());

    m_path = tmpRoot + L"\\FreeHeroes\\";

    if (!DirectoryExists(m_path.c_str()) && !CreateDirectory(m_path.c_str(), nullptr))
        throw std::runtime_error("Failed to create temporary path!");
}

TmpDir::~TmpDir()
{
    if (!m_doDelete)
        return;

    SHFILEOPSTRUCT dirOp;
    ZeroMemory(&dirOp, sizeof(SHFILEOPSTRUCT));

    dirOp.wFunc = FO_DELETE;

    m_path.back() = L'\0';
    dirOp.pFrom   = m_path.c_str();
    dirOp.fFlags  = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;

    SHFileOperation(&dirOp);
}
