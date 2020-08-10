/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
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

#include "StringUtils.hpp"

#include <json.hpp>

#include <cassert>

namespace FreeHeroes::Core::Reflection {

using namespace nlohmann;

bool deserialize(LibraryIdResolver & idResolver, LibraryFaction& faction, const json& jsonObj)
{
    deserializeFromJson(idResolver, faction, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver & idResolver, LibrarySecondarySkill& skill, const json& jsonObj)
{
    deserializeFromJson(idResolver, skill, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver & idResolver, LibraryUnit& unit, const json& jsonObj)
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

bool deserialize(LibraryIdResolver & idResolver, LibraryHeroSpec& spec, const json& jsonObj)
{
    deserializeFromJson(idResolver, spec, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver & idResolver, LibraryArtifact& artifact, const json& jsonObj)
{
    deserializeFromJson(idResolver, artifact, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver & idResolver, LibraryHero& hero, const json& jsonObj)
{
    deserializeFromJson(idResolver, hero, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver & idResolver, LibrarySpell& spell, const json& jsonObj)
{
    deserializeFromJson(idResolver, spell, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver & idResolver, LibraryResource& obj, const json& jsonObj)
{
    deserializeFromJson(idResolver, obj, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver & idResolver, LibraryTerrain& obj, const json& jsonObj)
{
    deserializeFromJson(idResolver, obj, jsonObj);
    return true;
}

bool deserialize(LibraryIdResolver & idResolver, LibraryMapObject& obj, const json& jsonObj)
{
    deserializeFromJson(idResolver, obj, jsonObj);

    return true;
}

bool deserialize(LibraryIdResolver & idResolver, SkillHeroItem & obj, const json& jsonObj)
{
    deserializeFromJson(idResolver, obj, jsonObj);
    return true;
}
bool serialize(const SkillHeroItem& obj, json& jsonObj)
{
    jsonObj = serializeToJson(obj);
    return true;
}

namespace {
class AbstractStringTransform : public IJsonTransform {
public:
    bool needTransform(const json & in) const noexcept override {
        return in.type() == json::value_t::string;
    }
};

class TraitsTransform : public IJsonTransform {
public:
    bool transform(const json & in, json & out) const override {
        for (const auto & elem : in)
            out[std::string(elem)] = true;
        return true;
    }
    bool needTransform(const json & in) const noexcept override {
        return in.type() == json::value_t::array;
    }
};

class ResourceAmountTransform : public AbstractStringTransform {
public:
    bool transform(const json & in, json & out) const override {
        out = json(json::value_t::object);
        std::string value(in);

        if (value.empty() || value == "0") {
            return true;
        }
        auto parts  = splitLine(value, ',', true);
        bool hasGold = false;
        for (size_t i = 0; i < parts.size(); ++i) {
            const auto & str = parts[i];
            auto partsEq  = splitLine(str, '=', true);
            if (partsEq.size() == 1 && !hasGold) {
                 out["gold"] = partsEq[0];
                 hasGold = true;
                 continue;
            }
            if (partsEq.size() != 2 || partsEq[0].empty()) {
                return {};
            }
            int value = std::atoi(partsEq[1].c_str());
            if (!value)
                return false;

            out[partsEq[0]] = value;
        }
        return true;
    }
};
class ArtifactRewardAmountTransform : public AbstractStringTransform {
public:
    bool transform(const json & in, json & out) const override {
        out = json(json::value_t::object);
        std::string value(in);

        if (value.empty()) {
            return true;
        }
        auto parts  = splitLine(value, ',', true);
        auto & resources = out["artifacts"];
        resources = json(json::value_t::array);
        for (size_t i = 0; i < parts.size(); ++i) {
            json res;
            const auto & str = parts[i];
            auto partsEq  = splitLine(str, '=', true);
            if (partsEq.size() != 2 || partsEq[0].empty()) {
                return {};
            }
            int value = std::atoi(partsEq[1].c_str());
            if (!value)
                return false;
            res["class"] = partsEq[0];
            res["n"] = value;
            resources.push_back(res);
        }
        return true;
    }
};

class UnitWithCountTransform : public AbstractStringTransform {
public:
    bool transform(const json & in, json & out) const override {
        //outArray = json(json::value_t::array);
        //for (const auto & in : inArray) {
        out = json(json::value_t::object);

        std::string value(in);
        if (value.empty()) {
            return false;
        }
        auto parts  = splitLine(value, '=', true);
        if (parts.size() != 2)
            return false;
        const int count = std::atoi(parts[1].c_str());
        if (count <= 0)
            return false;

        out["id"] = parts[0];
        out["n"] = count;
           // outArray.push_back(out);
        //}
        return true;
    }
};
class StartUnitTransform : public AbstractStringTransform {
public:
    bool transform(const json & in, json & out) const override {
        std::string value(in);
        auto parts  = splitLine(value, '=', true);
        if (parts.size() != 1 && parts.size() != 2)
            return false;

        out["id"] = parts[0];
        if (parts.size() == 1)
            return true;

        auto partsRange  = splitLine(parts[1], '-', true);
        if (partsRange.size() != 2)
            return false;

        const int min = std::atoi(partsRange[0].c_str());
        const int max = std::atoi(partsRange[1].c_str());
        if (min <= 0 || max <= 0)
            return false;

        out["stackSize"]["min"] = min;
        out["stackSize"]["max"] = max;
        return true;
    }
};
class ClassWeightsTransform : public IJsonTransform {
public:
    bool needTransform(const json & in) const noexcept override {
        return in.type() == json::value_t::object;
    }
    bool transform(const json & in, json & out) const override {
        out = json(json::value_t::array);
        for (auto it = in.begin(); it != in.end(); ++it)
        {
            json pair;
            pair["key"] = it.key();
            pair["value"] = it.value();
            out.push_back(pair);
        }
        return true;
    }
};
}
template<>
const IJsonTransform * getJsonTransform<UnitWithCount>() {
    static const UnitWithCountTransform transform;
    return &transform;
}
template<>
const IJsonTransform * getJsonTransform<LibraryUnit::Traits>() {
    static const TraitsTransform transform;
    return &transform;
}

template<>
const IJsonTransform * getJsonTransform<LibraryHero::StartStack>() {
    static const StartUnitTransform transform;
    return &transform;
}

template<>
const IJsonTransform * getJsonTransform<ResourceAmount>() {
    static const ResourceAmountTransform transform;
    return &transform;
}
template<>
const IJsonTransform * getJsonTransform<ArtifactRewardAmount>() {
    static const ArtifactRewardAmountTransform transform;
    return &transform;
}
template<>
const IJsonTransform * getJsonTransform<LibraryFactionHeroClass::SkillWeights>() {
    static const ClassWeightsTransform transform;
    return &transform;
}
}
