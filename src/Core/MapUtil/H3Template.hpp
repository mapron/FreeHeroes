/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/PropertyTree.hpp"
#include "MernelPlatform/FileFormatCSVTable.hpp"

#include <string>

namespace FreeHeroes {
using PropertyTree = Mernel::PropertyTree;
using CSVTable     = Mernel::CSVTable;

struct H3Template {
    std::string m_packName;
    bool        m_endsWithNL = false;

    std::vector<std::vector<std::string>> m_data;

    void readCSV(const CSVTable& table);
    void writeCSV(CSVTable& table) const;
    void toJson(PropertyTree& data) const;
    void fromJson(const PropertyTree& data);
};

}
