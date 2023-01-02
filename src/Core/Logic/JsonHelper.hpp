/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "MernelPlatform/PropertyTree.hpp"

namespace FreeHeroes::Core {

namespace details {

struct ColDesc {
    std::string          key;
    std::vector<ColDesc> children;

    void parseChildren(const Mernel::PropertyTreeList& cols)
    {
        children.reserve(cols.size());
        for (const auto& col : cols) {
            ColDesc child;
            if (col.isMap()) {
                assert(col.getMap().size() == 1);
                auto it2  = col.getMap().cbegin();
                child.key = it2->first;
                child.parseChildren(it2->second.getList());
            } else {
                child.key = col.getScalar().toString();
            }
            children.push_back(child);
        }
    }

    void applyValue(Mernel::PropertyTree& mainRecord, const Mernel::PropertyTree& packedRecord) const
    {
        if (!children.size()) {
            mainRecord = packedRecord;
        } else {
            assert(children.size() >= packedRecord.getList().size());
            size_t index = 0;
            for (const auto& child : children) {
                if (index >= packedRecord.getList().size())
                    break;
                child.applyValue(mainRecord[child.key], packedRecord.getList()[index++]);
            }
        }
    }
};

}

int addJsonObjectToIndex(Mernel::PropertyTree& index, const Mernel::PropertyTree& fileSections)
{
    int totalRecordsFound = 0;
    for (const auto& section : fileSections.getList()) {
        const std::string type            = section["scope"].getScalar().toString();
        auto&             recordObjectMap = index[type];
        if (section.contains("compactCols")) { // compressed records.

            const auto& cols = section["compactCols"].getList();
            const auto& rows = section["records"].getMap();

            details::ColDesc colDesc;
            colDesc.parseChildren(cols);

            for (auto it = rows.cbegin(); it != rows.cend(); ++it) {
                const std::string& id         = it->first;
                auto&              mainRecord = recordObjectMap[id];
                const auto&        colValues  = it->second;
                colDesc.applyValue(mainRecord, colValues);
                totalRecordsFound++;
            }

            continue;
        }
        if (section.contains("records")) { // non-compressed records.

            for (auto it = section["records"].getMap().cbegin(); it != section["records"].getMap().cend(); ++it) {
                const std::string& id = it->first;
                Mernel::PropertyTree::mergePatch(recordObjectMap[id], it->second);
                totalRecordsFound++;
            }
            continue;
        }

        assert(!"unknown file format");
    }
    return totalRecordsFound;
}

}
