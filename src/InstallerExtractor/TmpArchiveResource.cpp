/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "TmpArchiveResource.hpp"

#include "Extractor7z.hpp"

#include <Windows.h>

#include <memory>

TmpArchiveResource::TmpArchiveResource()
{
    HRSRC   hRes = FindResource(nullptr, L"archive", RT_RCDATA);
    HGLOBAL hMem = LoadResource(nullptr, hRes);
    void*   pMem = LockResource(hMem);
    DWORD   size = SizeofResource(nullptr, hRes);

    m_zip = std::make_unique<Extractor7z>(pMem, size);
}

void TmpArchiveResource::ExtractFiles(const std::wstring& dest)
{
    m_zip->ExtractFiles(dest);
}

TmpArchiveResource::~TmpArchiveResource()
{
    m_zip.reset();
}
