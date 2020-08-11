/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <json.hpp>


namespace FreeHeroes::Core {

namespace details {

struct ColDesc {
    std::string key;
    std::vector<ColDesc> children;

    void parseChildren(const nlohmann::json& cols) {
        children.reserve(cols.size());
        for (const auto & col : cols) {
            ColDesc child;
            if (col.is_object()) {
                assert(col.size() == 1);
                auto it2 = col.cbegin();
                child.key = it2.key();
                child.parseChildren(it2.value());
            } else {
                child.key = static_cast<std::string>(col);
            }
            children.push_back(child);
        }
    }

    void applyValue(nlohmann::json& mainRecord, const nlohmann::json& packedRecord) const {

        if (!children.size()) {
            mainRecord = packedRecord;
        } else {
            assert(children.size() >= packedRecord.size());
            size_t index = 0;
            for (const auto & child : children) {
                if (index >= packedRecord.size())
                    break;
                child.applyValue(mainRecord[child.key], packedRecord[index ++]);
            }
        }
    }
};

}

int addJsonObjectToIndex(nlohmann::json& index, const nlohmann::json& fileSections) {
    int totalRecordsFound = 0;
    for (const auto & section : fileSections) {
        const std::string type = section["scope"];
        auto & recordObjectMap = index[type];
        if (section.contains("compactCols")) { // compressed records.

            const auto& cols = section["compactCols"];
            const auto& rows = section["records"];

            details::ColDesc colDesc;
            colDesc.parseChildren(cols);

            for (auto it = rows.cbegin(); it != rows.cend(); ++it) {
                const std::string & id = it.key();
                auto & mainRecord = recordObjectMap[id];
                const auto & colValues = it.value();
                colDesc.applyValue(mainRecord, colValues);
                totalRecordsFound++;
            }

            continue;
        }
        if (section.contains("records")) { // non-compressed records.

            for (auto it = section["records"].cbegin(); it != section["records"].cend(); ++it) {
                const std::string & id = it.key();
                recordObjectMap[id].merge_patch(it.value());
                totalRecordsFound++;
            }
            continue;
        }

        assert(!"unknown file format");
    }
    return totalRecordsFound;
}

}
