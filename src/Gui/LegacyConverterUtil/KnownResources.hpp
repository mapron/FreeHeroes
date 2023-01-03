/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/FsUtils.hpp"

#include "MernelPlatform/PropertyTree.hpp"

#include "LegacyConverterUtilExport.hpp"

#include <string_view>
#include <vector>
#include <unordered_map>

namespace FreeHeroes {

struct KnownResource {
    std::string legacyId;
    std::string newId;
    std::string destinationSubfolder;
};

class LEGACYCONVERTERUTIL_EXPORT KnownResources {
public:
    KnownResources(const Mernel::std_path& config, const Mernel::std_path& ppConfig);
    const KnownResource* find(const std::string& legacyId) const;
    Mernel::PropertyTree findPP(const std::string& legacyId) const;

private:
    std::vector<KnownResource>                            m_resources;
    std::unordered_map<std::string, const KnownResource*> m_index;

    std::unordered_map<std::string, Mernel::PropertyTree> m_ppData;
};

}
