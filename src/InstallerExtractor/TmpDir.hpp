/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <string>

class TmpDir {
public:
    const std::wstring& path() const;

    TmpDir();
    ~TmpDir();

    void keep() { m_doDelete = false; }

private:
    bool         m_doDelete = true;
    std::wstring m_path;
};
