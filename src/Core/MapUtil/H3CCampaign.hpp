/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "H3MMap.hpp"

namespace FreeHeroes {

struct H3CCampaign {
    uint32_t    m_version = 0;
    uint8_t     m_campId  = 0;
    std::string m_name;
    std::string m_description;
    uint8_t     m_musicId                   = 0;
    uint8_t     m_difficultyChoosenByPlayer = 0;

    struct Scenario {
        std::string             m_filename;
        Mernel::ByteArrayHolder m_data;
    };
    std::vector<Scenario> m_scenarios;

    H3CCampaign();

    void readBinary(ByteOrderDataStreamReader& stream);
    void writeBinary(ByteOrderDataStreamWriter& stream) const;
    void toJson(PropertyTree& data) const;
    void fromJson(const PropertyTree& data);
};

}
