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

    struct PrologEpilog {
        uint8_t     m_enabled = 0;
        uint8_t     m_video   = 0;
        uint8_t     m_music   = 0;
        std::string m_text;

        void readBinary(ByteOrderDataStreamReader& stream);
        void writeBinary(ByteOrderDataStreamWriter& stream) const;
    };

    struct Scenario {
        uint32_t m_version        = 0;
        uint32_t m_totalScenarios = 0; // need to be filled before reading.

        std::string          m_filename;
        std::uint32_t        m_packedMapSize = 0;
        std::vector<uint8_t> m_preconditions;
        uint8_t              m_regionColor = 0;
        uint8_t              m_difficulty  = 0;
        std::string          m_regionText;
        PrologEpilog         m_prolog;
        PrologEpilog         m_epilog;

        void readBinary(ByteOrderDataStreamReader& stream);
        void writeBinary(ByteOrderDataStreamWriter& stream) const;

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
