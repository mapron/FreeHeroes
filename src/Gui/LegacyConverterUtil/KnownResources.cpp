/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "KnownResources.hpp"

#include "MernelPlatform/FileIOUtils.hpp"
#include "MernelPlatform/FileFormatJson.hpp"

namespace FreeHeroes {

const KnownResource* KnownResources::find(const std::string& legacyId) const
{
    auto it = m_index.find(legacyId);
    return it == m_index.cend() ? nullptr : it->second;
}

Mernel::PropertyTree KnownResources::findPP(const std::string& legacyId) const
{
    auto it = m_ppData.find(legacyId);
    return it == m_ppData.cend() ? Mernel::PropertyTree{} : it->second;
}

KnownResources::KnownResources(const Mernel::std_path& config, const Mernel::std_path& ppConfig)
{
    {
        std::string buffer   = Mernel::readFileIntoBuffer(config);
        auto        jsonData = Mernel::readJsonFromBuffer(buffer);
        m_resources.reserve(10000);

        for (const auto& [key, row] : jsonData.getMap()) {
            const auto    rowArr = row.getList();
            KnownResource res;
            res.legacyId = key;
            rowArr[0].getScalar().convertTo(res.newId);
            rowArr[1].getScalar().convertTo(res.destinationSubfolder);
            m_resources.push_back(std::move(res));
        }

        for (const auto& res : m_resources) {
            m_index[res.legacyId] = &res;
        }
    }

    {
        std::string buffer   = Mernel::readFileIntoBuffer(ppConfig);
        auto        jsonData = Mernel::readJsonFromBuffer(buffer);

        for (const auto& [key, row] : jsonData.getMap()) {
            m_ppData[key] = Mernel::PropertyTree(row.getMap());
        }
    }
}

}
