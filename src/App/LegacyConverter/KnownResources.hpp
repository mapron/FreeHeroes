/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "FsUtils.hpp"

#include <string_view>
#include <vector>
#include <unordered_map>

namespace FreeHeroes::Conversion {

struct KnownResource {
    std::string              legacyId;
    std::string              newId;
    std::string              destinationSubfolder;
    std::string              filenameReplace;
    std::string              postprocessUtility;
    std::vector<std::string> params;
};

class KnownResources {
public:
    KnownResources(const Core::std_path& config);
    const KnownResource* find(const std::string& legacyId) const;

private:
    std::vector<KnownResource>                            m_resources;
    std::unordered_map<std::string, const KnownResource*> m_index;
};

}
