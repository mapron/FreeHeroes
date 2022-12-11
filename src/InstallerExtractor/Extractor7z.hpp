/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once
#include <string>
#include <memory>
#include <set>
#include <map>
#include <vector>
#include <functional>

class Extractor7zImpl;

class Extractor7z {
public:
    Extractor7z(void* data, size_t size);
    ~Extractor7z();

    void ExtractFiles(const std::wstring& destPath);

private:
    void CreateFolders(std::wstring fname, const std::wstring& destPath);
    void ExtractFilenames();

    std::unique_ptr<Extractor7zImpl> m_impl;
};
