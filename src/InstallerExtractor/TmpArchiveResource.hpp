/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <memory>
#include <string>
#include <set>

using HANDLE = void*;
class Extractor7z;

class TmpArchiveResource {
    std::wstring                 m_filename;
    std::unique_ptr<Extractor7z> m_zip;

public:
    TmpArchiveResource();
    void ExtractFiles(const std::wstring& dest);
    ~TmpArchiveResource();
};
