/*
 * Copyright (C) 2024 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */

#include "H3Template.hpp"

#include "H3TemplateReflection.hpp"

#include "MernelReflection/PropertyTreeReader.hpp"
#include "MernelReflection/PropertyTreeWriter.hpp"

namespace FreeHeroes {

void H3Template::readCSV(const CSVTable& table)
{
    if (table.rows.empty())
        throw std::runtime_error("Empty csv file");

    m_packName = table.rows[0].data[0].str;
    m_data.resize(table.rows.size());
    for (size_t i = 0; i < m_data.size(); ++i) {
        m_data[i].resize(table.rows[i].data.size());
        for (size_t j = 0; j < m_data[i].size(); ++j) {
            m_data[i][j] = table.rows[i].data[j].str;
        }
    }
}

void H3Template::writeCSV(CSVTable& table) const
{
    table.rows.resize(m_data.size());
    for (size_t i = 0; i < m_data.size(); ++i) {
        table.rows[i].data.resize(m_data[i].size());
        for (size_t j = 0; j < m_data[i].size(); ++j) {
            table.rows[i].data[j].str = m_data[i][j];
        }
    }
}

void H3Template::toJson(PropertyTree& data) const
{
    Mernel::Reflection::PropertyTreeWriter writer;
    writer.valueToJson(*this, data);
}

void H3Template::fromJson(const PropertyTree& data)
{
    Mernel::Reflection::PropertyTreeReader reader;
    *this = {};
    reader.jsonToValue(data, *this);
}

}
