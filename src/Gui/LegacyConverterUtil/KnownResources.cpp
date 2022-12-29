/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "KnownResources.hpp"

#include "FileIOUtils.hpp"
#include "FileFormatJson.hpp"

namespace FreeHeroes::Conversion {

const KnownResource* KnownResources::find(const std::string& legacyId) const
{
    auto it = m_index.find(legacyId);
    return it == m_index.cend() ? nullptr : it->second;
}

KnownResources::KnownResources(const Core::std_path& config)
{
    std::string buffer   = Core::readFileIntoBufferThrow(config);
    auto        jsonData = Core::readJsonFromBufferThrow(buffer);
    m_resources.reserve(10000);

    for (const auto& [key, row] : jsonData.getMap()) {
        const auto    rowArr = row.getList();
        KnownResource res;
        res.legacyId = key;
        rowArr[0].getScalar().convertTo(res.newId);
        rowArr[1].getScalar().convertTo(res.destinationSubfolder);
        rowArr[2].getScalar().convertTo(res.filenameReplace);
        if (rowArr.size() > 3) {
            res.handlers = rowArr[3].getMap();
        }
        m_resources.push_back(std::move(res));
    }

    for (const auto& res : m_resources) {
        m_index[res.legacyId] = &res;
    }
}

}
