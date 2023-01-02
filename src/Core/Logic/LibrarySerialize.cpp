/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LibrarySerialize.hpp"
#include "LibraryReflection.hpp"

#include "GameDatabasePropertyReader.hpp"
#include "GameDatabasePropertyWriter.hpp"

#include "MernelPlatform/StringUtils.hpp"

#include "MernelPlatform/PropertyTree.hpp"

#include <cassert>

namespace FreeHeroes::Core {
using namespace Mernel;
using namespace Reflection;

bool deserialize(const IGameDatabase* gameDatabase, LibraryArtifact& artifact, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, artifact);
    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, LibraryDwelling& dwelling, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, dwelling);
    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, LibraryFaction& faction, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, faction);
    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, LibraryHero& hero, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, hero);
    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, LibraryHeroSpec& spec, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, spec);
    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, LibraryMapBank& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);

    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, LibraryMapObstacle& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);

    return true;
}
bool deserialize(const IGameDatabase* gameDatabase, LibraryMapVisitable& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);

    return true;
}
bool deserialize(const IGameDatabase* gameDatabase, LibraryObjectDef& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);
    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, LibraryResource& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);
    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, LibrarySecondarySkill& skill, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, skill);
    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, LibrarySpell& spell, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, spell);
    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, LibraryTerrain& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);
    {
        int offset = 0;
        using BT   = LibraryTerrain::BorderType;
        for (auto bt : { BT::TL, BT::L, BT::T, BT::BR, BT::TLS, BT::BRS }) {
            const int count                          = obj.presentationParams.borderCounts[bt];
            obj.presentationParams.borderOffsets[bt] = offset;
            int rotationCount                        = 1;
            if (obj.presentationParams.hasRotations) {
                rotationCount = 4;
                if (bt == BT::L || bt == BT::T)
                    rotationCount = 2;
            }
            offset += count * rotationCount;
        }
    }
    {
        int offset = 0;
        using BT   = LibraryTerrain::BorderType;
        for (auto bt : {
                 BT::Mixed_DNND,
                 BT::Mixed_DNNS,
                 BT::Mixed_SNNS,
                 BT::Mixed_NDSD,
                 BT::Mixed_NSDD,
                 BT::Mixed_NDNS,
                 BT::Mixed_NNDS,
                 BT::Mixed_NSDS,
                 BT::Mixed_NDSS,
             }) {
            const int count                               = obj.presentationParams.borderMixedCounts[bt];
            obj.presentationParams.borderMixedOffsets[bt] = offset;
            offset += count;
        }
    }
    {
        int offset = 0;
        using BT   = LibraryTerrain::BorderType;
        for (auto bt : {
                 BT::Special_DDDD,
                 BT::Special_SSSS,
                 BT::Special_DDDS,
                 BT::Special_SSSD,
                 BT::Special_NSSD,
                 BT::Special_NDDS,
             }) {
            const int count                                 = obj.presentationParams.borderSpecialCounts[bt];
            obj.presentationParams.borderSpecialOffsets[bt] = offset;
            offset += count;
        }
    }
    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, LibraryUnit& unit, const PropertyTree& jsonObj)
{
    using namespace FreeHeroes::Core;

    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, unit);
    if (unit.primary.armySpeed == 0)
        unit.primary.armySpeed = unit.primary.battleSpeed;

    // some checks

    if (unit.traits.rangeAttack)
        assert(unit.primary.shoots > 0);

    return true;
}
bool deserialize(const IGameDatabase* gameDatabase, LibraryGameRules& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);
    return true;
}

bool deserialize(const IGameDatabase* gameDatabase, SkillHeroItem& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReaderDatabase reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);
    return true;
}

bool serialize(const SkillHeroItem& obj, PropertyTree& jsonObj)
{
    PropertyTreeWriterDatabase writer;
    writer.valueToJson(obj, jsonObj);
    return true;
}

}

namespace Mernel::Reflection {

template<>
bool MetaInfo::transformTreeRead<LibraryUnit::Traits>(const PropertyTree& treeIn, PropertyTree& treeOut)
{
    if (!treeIn.isList())
        return false;
    treeOut.convertToMap();
    for (const auto& elem : treeIn.getList())
        treeOut[std::string(elem.getScalar().toString())] = PropertyTreeScalar(true);
    return true;
}

template<>
bool MetaInfo::transformTreeRead<ResourceAmount>(const PropertyTree& treeIn, PropertyTree& treeOut)
{
    if (!treeIn.isScalar())
        return false;

    treeOut.convertToMap();
    std::string valueStr(treeIn.getScalar().toString());

    if (valueStr.empty() || valueStr == "0") {
        return true;
    }
    auto parts   = splitLine(valueStr, ',', true);
    bool hasGold = false;
    for (size_t i = 0; i < parts.size(); ++i) {
        const auto& str     = parts[i];
        auto        partsEq = splitLine(str, '=', true);
        if (partsEq.size() == 1 && !hasGold) {
            treeOut["gold"] = PropertyTreeScalar(std::atoi(partsEq[0].c_str()));
            hasGold         = true;
            continue;
        }
        if (partsEq.size() != 2 || partsEq[0].empty()) {
            return {};
        }
        int value = std::atoi(partsEq[1].c_str());
        if (!value)
            return false;

        treeOut[partsEq[0]] = PropertyTreeScalar(value);
    }
    return true;
}

template<>
bool MetaInfo::transformTreeRead<UnitWithCount>(const PropertyTree& treeIn, PropertyTree& treeOut)
{
    if (!treeIn.isScalar())
        return false;

    treeOut.convertToMap();

    std::string value(treeIn.getScalar().toString());
    if (value.empty()) {
        return false;
    }
    auto parts = splitLine(value, '=', true);
    if (parts.size() != 2)
        return false;
    const int count = std::atoi(parts[1].c_str());
    if (count <= 0)
        return false;

    treeOut["id"] = PropertyTreeScalar(parts[0]);
    treeOut["n"]  = PropertyTreeScalar(count);

    return true;
}
template<>
bool MetaInfo::transformTreeRead<LibraryHero::StartStack>(const PropertyTree& treeIn, PropertyTree& treeOut)
{
    if (!treeIn.isScalar())
        return false;
    std::string value(treeIn.getScalar().toString());
    auto        parts = splitLine(value, '=', true);
    if (parts.size() != 1 && parts.size() != 2)
        return false;

    treeOut["id"] = PropertyTreeScalar(parts[0]);
    if (parts.size() == 1)
        return true;

    auto partsRange = splitLine(parts[1], '-', true);
    if (partsRange.size() != 2)
        return false;

    const int min = std::atoi(partsRange[0].c_str());
    const int max = std::atoi(partsRange[1].c_str());
    if (min <= 0 || max <= 0)
        return false;

    treeOut["stackSize"]["min"] = PropertyTreeScalar(min);
    treeOut["stackSize"]["max"] = PropertyTreeScalar(max);
    return true;
}

template<>
bool MetaInfo::transformTreeRead<ObjectDefMappings>(const PropertyTree& treeIn, PropertyTree& treeOut)
{
    if (!treeIn.isScalar())
        return false;

    treeOut.convertToMap();
    auto s = treeIn.getScalar().toString();
    if (s.empty())
        return true;

    treeOut["m"][""] = PropertyTreeScalar(std::move(s));
    return true;
}

}
