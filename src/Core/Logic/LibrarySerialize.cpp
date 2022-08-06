/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LibrarySerialize.hpp"
#include "LibraryReflection.hpp"

#include "Reflection/PropertyTreeReader.hpp"
#include "Reflection/PropertyTreeWriter.hpp"

#include "IGameDatabase.hpp"

#include "StringUtils.hpp"

#include "PropertyTree.hpp"

#include <cassert>

namespace FreeHeroes::Core::Reflection {

bool deserialize(IGameDatabase& gameDatabase, LibraryFaction& faction, const PropertyTree& jsonObj)
{
    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, faction);
    return true;
}

bool deserialize(IGameDatabase& gameDatabase, LibrarySecondarySkill& skill, const PropertyTree& jsonObj)
{
    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, skill);
    return true;
}

bool deserialize(IGameDatabase& gameDatabase, LibraryUnit& unit, const PropertyTree& jsonObj)
{
    using namespace FreeHeroes::Core;

    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, unit);
    if (unit.primary.armySpeed == 0)
        unit.primary.armySpeed = unit.primary.battleSpeed;

    // some checks

    if (unit.traits.rangeAttack)
        assert(unit.primary.shoots > 0);

    return true;
}

bool deserialize(IGameDatabase& gameDatabase, LibraryHeroSpec& spec, const PropertyTree& jsonObj)
{
    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, spec);
    return true;
}

bool deserialize(IGameDatabase& gameDatabase, LibraryArtifact& artifact, const PropertyTree& jsonObj)
{
    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, artifact);
    return true;
}

bool deserialize(IGameDatabase& gameDatabase, LibraryHero& hero, const PropertyTree& jsonObj)
{
    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, hero);
    return true;
}

bool deserialize(IGameDatabase& gameDatabase, LibrarySpell& spell, const PropertyTree& jsonObj)
{
    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, spell);
    return true;
}

bool deserialize(IGameDatabase& gameDatabase, LibraryResource& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);
    return true;
}

bool deserialize(IGameDatabase& gameDatabase, LibraryTerrain& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);
    return true;
}

bool deserialize(IGameDatabase& gameDatabase, LibraryMapObject& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);

    return true;
}

bool deserialize(IGameDatabase& gameDatabase, SkillHeroItem& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);
    return true;
}
bool deserialize(IGameDatabase& gameDatabase, LibraryGameRules& obj, const PropertyTree& jsonObj)
{
    PropertyTreeReader reader(gameDatabase);
    reader.jsonToValue(jsonObj, obj);
    return true;
}

bool serialize(const SkillHeroItem& obj, PropertyTree& jsonObj)
{
    PropertyTreeWriter writer;
    jsonObj = writer.valueToJson(obj);
    return true;
}

template<>
BonusRatio MetaInfo::fromString(const std::string& value)
{
    auto parts = splitLine(value, '/', true);
    if (parts.size() == 2) {
        int n = std::atoi(parts[0].c_str());
        int d = std::atoi(parts[1].c_str());
        if (d != 0)
            return { n, d };
    }
    return {};
}
template<>
bool MetaInfo::transformTree<LibraryUnit::Traits>(const PropertyTree& treeIn, PropertyTree& treeOut)
{
    if (!treeIn.isList())
        return false;
    treeOut.convertToMap();
    for (const auto& elem : treeIn.getList())
        treeOut[std::string(elem.getScalar().toString())] = PropertyTreeScalar(true);
    return true;
}

template<>
bool MetaInfo::transformTree<ResourceAmount>(const PropertyTree& treeIn, PropertyTree& treeOut)
{
    if (!treeIn.isScalar())
        return false;

    treeOut.convertToMap();
    std::string value(treeIn.getScalar().toString());

    if (value.empty() || value == "0") {
        return true;
    }
    auto parts   = splitLine(value, ',', true);
    bool hasGold = false;
    for (size_t i = 0; i < parts.size(); ++i) {
        const auto& str     = parts[i];
        auto        partsEq = splitLine(str, '=', true);
        if (partsEq.size() == 1 && !hasGold) {
            treeOut["gold"] = PropertyTreeScalar(partsEq[0]);
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
bool MetaInfo::transformTree<ArtifactRewardAmount>(const PropertyTree& treeIn, PropertyTree& treeOut)
{
    if (!treeIn.isScalar())
        return false;

    treeOut.convertToMap();
    std::string value(treeIn.getScalar().toString());

    if (value.empty()) {
        return true;
    }
    auto  parts     = splitLine(value, ',', true);
    auto& resources = treeOut["artifacts"];
    resources.convertToList();
    for (size_t i = 0; i < parts.size(); ++i) {
        PropertyTree res;
        const auto&  str     = parts[i];
        auto         partsEq = splitLine(str, '=', true);
        if (partsEq.size() != 2 || partsEq[0].empty()) {
            return {};
        }
        int value = std::atoi(partsEq[1].c_str());
        if (!value)
            return false;
        res["class"] = PropertyTreeScalar(partsEq[0]);
        res["n"]     = PropertyTreeScalar(value);
        resources.append(res);
    }
    return true;
}
template<>
bool MetaInfo::transformTree<UnitWithCount>(const PropertyTree& treeIn, PropertyTree& treeOut)
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
bool MetaInfo::transformTree<LibraryHero::StartStack>(const PropertyTree& treeIn, PropertyTree& treeOut)
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

}
