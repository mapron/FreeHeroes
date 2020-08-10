/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "KnownResources.hpp"

#include "StringUtils.hpp"

#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>

namespace FreeHeroes::Conversion {

const KnownResource * KnownResources::find(const std::string & legacyId) const {
    auto it = m_index.find(legacyId);
    return it == m_index.cend() ? nullptr : it->second;
}


KnownResources::KnownResources(const Core::std_path& config)
{
    std::ifstream ifs(config, std::ios::in | std::ios::binary);
    std::string line;
    m_resources.reserve(10000);
    std::vector<std::string> tokens;
    std::vector<std::string> params;

    while(std::getline(ifs, line)) {
        if (line.empty())
            continue;
        tokens = Core::splitLine(line, '\t', true);
        if (tokens.empty())
            continue;
        if (tokens.size() < 6)
            tokens.resize(6);

        params = Core::splitLine(tokens[5], ';', true);
        KnownResource res{tokens[0], tokens[1], tokens[2], tokens[3], tokens[4], params};
        m_resources.emplace_back(std::move(res));
    }
    for (const auto & res : m_resources)
        m_index[res.legacyId] = &res;
}


}
