/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "LibrarySerialize.hpp"
#include "LibraryReflection.hpp"

#include "JsonRTTRDeserialize.hpp"
#include "JsonRTTRSerialize.hpp"

#include "LibraryFaction.hpp"
#include "LibrarySecondarySkill.hpp"
#include "LibraryUnit.hpp"
#include "LibraryHeroSpec.hpp"
#include "LibraryArtifact.hpp"
#include "LibraryHero.hpp"
#include "LibrarySpell.hpp"
#include "LibraryTerrain.hpp"
#include "LibraryResource.hpp"
#include "LibraryMapObject.hpp"
#include "LibraryGameRules.hpp"

#include "StringUtils.hpp"

#include "PropertyTree.hpp"

#include <cassert>

namespace FreeHeroes::Core::Reflection {

bool deserialize(LibraryIdResolver& idResolver, LibraryFaction& faction, const PropertyTree& jsonObj)
{
    deserializeFromJson(idResolver, faction, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver& idResolver, LibrarySecondarySkill& skill, const PropertyTree& jsonObj)
{
    deserializeFromJson(idResolver, skill, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver& idResolver, LibraryUnit& unit, const PropertyTree& jsonObj)
{
    using namespace FreeHeroes::Core;

    deserializeFromJson(idResolver, unit, jsonObj);
    if (unit.primary.armySpeed == 0)
        unit.primary.armySpeed = unit.primary.battleSpeed;

    // some checks

    if (unit.traits.rangeAttack)
        assert(unit.primary.shoots > 0);

    return true;
}

bool deserialize(LibraryIdResolver& idResolver, LibraryHeroSpec& spec, const PropertyTree& jsonObj)
{
    deserializeFromJson(idResolver, spec, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver& idResolver, LibraryArtifact& artifact, const PropertyTree& jsonObj)
{
    deserializeFromJson(idResolver, artifact, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver& idResolver, LibraryHero& hero, const PropertyTree& jsonObj)
{
    deserializeFromJson(idResolver, hero, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver& idResolver, LibrarySpell& spell, const PropertyTree& jsonObj)
{
    deserializeFromJson(idResolver, spell, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver& idResolver, LibraryResource& obj, const PropertyTree& jsonObj)
{
    deserializeFromJson(idResolver, obj, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver& idResolver, LibraryTerrain& obj, const PropertyTree& jsonObj)
{
    deserializeFromJson(idResolver, obj, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver& idResolver, LibraryMapObject& obj, const PropertyTree& jsonObj)
{
    deserializeFromJson(idResolver, obj, jsonObj);

    return true;
}

bool deserialize(LibraryIdResolver& idResolver, SkillHeroItem& obj, const PropertyTree& jsonObj)
{
    deserializeFromJson(idResolver, obj, jsonObj);
    return true;
}
bool deserialize(LibraryIdResolver& idResolver, LibraryGameRules& obj, const PropertyTree& jsonObj)
{
    deserializeFromJson(idResolver, obj, jsonObj);
    return true;
}

bool serialize(const SkillHeroItem& obj, PropertyTree& jsonObj)
{
    jsonObj = serializeToJson(obj);
    return true;
}

namespace {
class AbstractStringTransform : public IJsonTransform {
public:
    bool needTransform(const PropertyTree& in) const noexcept override
    {
        return in.isScalar() && in.getScalar().isString();
    }
};

class TraitsTransform : public IJsonTransform {
public:
    bool transform(const PropertyTree& in, PropertyTree& out) const override
    {
        for (const auto& elem : in.getList())
            out[std::string(elem.getScalar().toString())] = PropertyTreeScalar(true);
        return true;
    }
    bool needTransform(const PropertyTree& in) const noexcept override
    {
        return in.isList();
    }
};

class ResourceAmountTransform : public AbstractStringTransform {
public:
    bool transform(const PropertyTree& in, PropertyTree& out) const override
    {
        out.convertToMap();
        std::string value(in.getScalar().toString());

        if (value.empty() || value == "0") {
            return true;
        }
        auto parts   = splitLine(value, ',', true);
        bool hasGold = false;
        for (size_t i = 0; i < parts.size(); ++i) {
            const auto& str     = parts[i];
            auto        partsEq = splitLine(str, '=', true);
            if (partsEq.size() == 1 && !hasGold) {
                out["gold"] = PropertyTreeScalar(partsEq[0]);
                hasGold     = true;
                continue;
            }
            if (partsEq.size() != 2 || partsEq[0].empty()) {
                return {};
            }
            int value = std::atoi(partsEq[1].c_str());
            if (!value)
                return false;

            out[partsEq[0]] = PropertyTreeScalar(value);
        }
        return true;
    }
};
class ArtifactRewardAmountTransform : public AbstractStringTransform {
public:
    bool transform(const PropertyTree& in, PropertyTree& out) const override
    {
        out.convertToMap();
        std::string value(in.getScalar().toString());

        if (value.empty()) {
            return true;
        }
        auto  parts     = splitLine(value, ',', true);
        auto& resources = out["artifacts"];
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
};

class UnitWithCountTransform : public AbstractStringTransform {
public:
    bool transform(const PropertyTree& in, PropertyTree& out) const override
    {
        //outArray = json(json::value_t::array);
        //for (const auto & in : inArray) {
        out.convertToMap();

        std::string value(in.getScalar().toString());
        if (value.empty()) {
            return false;
        }
        auto parts = splitLine(value, '=', true);
        if (parts.size() != 2)
            return false;
        const int count = std::atoi(parts[1].c_str());
        if (count <= 0)
            return false;

        out["id"] = PropertyTreeScalar(parts[0]);
        out["n"]  = PropertyTreeScalar(count);
        // outArray.push_back(out);
        //}
        return true;
    }
};
class StartUnitTransform : public AbstractStringTransform {
public:
    bool transform(const PropertyTree& in, PropertyTree& out) const override
    {
        std::string value(in.getScalar().toString());
        auto        parts = splitLine(value, '=', true);
        if (parts.size() != 1 && parts.size() != 2)
            return false;

        out["id"] = PropertyTreeScalar(parts[0]);
        if (parts.size() == 1)
            return true;

        auto partsRange = splitLine(parts[1], '-', true);
        if (partsRange.size() != 2)
            return false;

        const int min = std::atoi(partsRange[0].c_str());
        const int max = std::atoi(partsRange[1].c_str());
        if (min <= 0 || max <= 0)
            return false;

        out["stackSize"]["min"] = PropertyTreeScalar(min);
        out["stackSize"]["max"] = PropertyTreeScalar(max);
        return true;
    }
};
class ClassWeightsTransform : public IJsonTransform {
public:
    bool needTransform(const PropertyTree& in) const noexcept override
    {
        return in.isMap();
    }
    bool transform(const PropertyTree& in, PropertyTree& out) const override
    {
        out.convertToList();
        for (auto it = in.getMap().cbegin(); it != in.getMap().cend(); ++it) {
            PropertyTreeMap pair;
            pair["key"]   = PropertyTreeScalar(it->first);
            pair["value"] = it->second;
            out.append(pair);
        }
        return true;
    }
};
}
template<>
const IJsonTransform* getJsonTransform<UnitWithCount>()
{
    static const UnitWithCountTransform transform;
    return &transform;
}
template<>
const IJsonTransform* getJsonTransform<LibraryUnit::Traits>()
{
    static const TraitsTransform transform;
    return &transform;
}

template<>
const IJsonTransform* getJsonTransform<LibraryHero::StartStack>()
{
    static const StartUnitTransform transform;
    return &transform;
}

template<>
const IJsonTransform* getJsonTransform<ResourceAmount>()
{
    static const ResourceAmountTransform transform;
    return &transform;
}
template<>
const IJsonTransform* getJsonTransform<ArtifactRewardAmount>()
{
    static const ArtifactRewardAmountTransform transform;
    return &transform;
}
template<>
const IJsonTransform* getJsonTransform<LibraryFactionHeroClass::SkillWeights>()
{
    static const ClassWeightsTransform transform;
    return &transform;
}

}
