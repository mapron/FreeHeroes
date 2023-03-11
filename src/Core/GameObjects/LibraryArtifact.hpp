/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "TranslationMap.hpp"
#include "LibraryFwd.hpp"
#include "LibrarySpell.hpp"
#include "LibraryObjectDef.hpp"

#include <string>
#include <vector>
#include <set>

#include <cassert>

namespace FreeHeroes::Core {

// clang-format off
enum class ArtifactSlotType { Sword, Shield, Helm, Torso,
                              Ring, Neck, Boots, Cape, Misc,
                              BmShoot, BmAmmo, BmTent,
                              Ring1, Misc1, Misc2, Misc3, Misc4,
                              Invalid, BagOnly };
// clang-format on

struct ArtifactWearingSet {
    std::set<ArtifactSlotType> wearingSlots;

    static ArtifactSlotType getCompatibleSlotType(ArtifactSlotType wearingSlot)
    {
        ArtifactSlotType compatSlot = wearingSlot;
        if (compatSlot == ArtifactSlotType::Misc1
            || compatSlot == ArtifactSlotType::Misc2
            || compatSlot == ArtifactSlotType::Misc3
            || compatSlot == ArtifactSlotType::Misc4)
            compatSlot = ArtifactSlotType::Misc;
        if (compatSlot == ArtifactSlotType::Ring1)
            compatSlot = ArtifactSlotType::Ring;
        return compatSlot;
    }
    static std::vector<ArtifactSlotType> getCompatibleWearingSlots(ArtifactSlotType compatSlot)
    {
        if (compatSlot == ArtifactSlotType::Misc)
            return { ArtifactSlotType::Misc, ArtifactSlotType::Misc1, ArtifactSlotType::Misc2, ArtifactSlotType::Misc3, ArtifactSlotType::Misc4 };
        if (compatSlot == ArtifactSlotType::Ring)
            return { ArtifactSlotType::Ring, ArtifactSlotType::Ring1 };
        return { compatSlot };
    }
    ArtifactWearingSet operator+(const ArtifactWearingSet& rh) const
    {
        ArtifactWearingSet result = *this;
        for (auto& slot : rh.wearingSlots)
            result.wearingSlots.insert(slot);
        return result;
    }
    ArtifactWearingSet operator-(const ArtifactWearingSet& rh) const
    {
        ArtifactWearingSet result;
        for (auto& slot : wearingSlots)
            if (!rh.wearingSlots.contains(slot))
                result.wearingSlots.insert(slot);
        return result;
    }
    static const ArtifactWearingSet& defaultFreeSlots()
    {
        using A = ArtifactSlotType;
        // clang-format off
        static const ArtifactWearingSet heroSlots{{A::Sword, A::Shield, A::Helm, A::Torso,
                        A::Ring, A::Neck, A::Boots, A::Cape, A::Misc,
                        A::Ring1, A::Misc1, A::Misc2, A::Misc3, A::Misc4}};
        // clang-format on
        return heroSlots;
    }
};

struct ArtifactSlotRequirement {
    std::map<ArtifactSlotType, int> compatCount;
    bool                            canFitInto(const ArtifactSlotRequirement& containerFree) const
    {
        for (const auto& p : compatCount) {
            ArtifactSlotType type = p.first;
            if (!containerFree.compatCount.contains(type))
                return false;
            const int thisCount      = p.second;
            const int containerCount = containerFree.compatCount.at(type);
            if (thisCount > containerCount)
                return false;
        }
        return true;
    }
    static const ArtifactSlotRequirement& defaultFreeSlots()
    {
        using A = ArtifactSlotType;
        // clang-format off
        static const ArtifactSlotRequirement heroSlots{
            {{A::Sword,1}, {A::Shield,1}, {A::Helm,1}, {A::Torso,1},
             {A::Ring,2}, {A::Neck,1}, {A::Boots,1}, {A::Cape,1}, {A::Misc,5}}
        };
        // clang-format on
        return heroSlots;
    }
    void subtract(const ArtifactSlotRequirement& req)
    {
        for (const auto& p : req.compatCount)
            compatCount[p.first] -= p.second;
    }
    void subtract(ArtifactSlotType compatSlot)
    {
        compatCount[compatSlot]--;
    }
    void add(const ArtifactSlotRequirement& req)
    {
        for (const auto& p : req.compatCount)
            compatCount[p.first] += p.second;
    }
    void add(ArtifactSlotType compatSlot)
    {
        compatCount[compatSlot]++;
    }
    ArtifactSlotRequirement operator-(const ArtifactSlotRequirement& rh) const
    {
        ArtifactSlotRequirement result = *this;
        result.subtract(rh);
        return result;
    }
    ArtifactSlotRequirement operator-(const ArtifactSlotType& rh) const
    {
        ArtifactSlotRequirement result = *this;
        result.subtract(rh);
        return result;
    }
    ArtifactSlotRequirement operator+(const ArtifactSlotRequirement& rh) const
    {
        ArtifactSlotRequirement result = *this;
        result.add(rh);
        return result;
    }
    ArtifactSlotRequirement operator+(const ArtifactSlotType& rh) const
    {
        ArtifactSlotRequirement result = *this;
        result.add(rh);
        return result;
    }

    bool getWearingTypes(ArtifactWearingSet& result, const ArtifactWearingSet& bannedSlots) const
    {
        for (const auto& p : compatCount) {
            int count = p.second;
            assert(count >= 0);
            if (count == 0)
                continue;

            auto wearingList = ArtifactWearingSet::getCompatibleWearingSlots(p.first);
            if (count > (int) wearingList.size())
                return false;
            int remain = count;
            for (auto candidate : wearingList) {
                if (bannedSlots.wearingSlots.contains(candidate))
                    continue;
                result.wearingSlots.insert(candidate);
                remain--;
                if (remain == 0)
                    break;
            }
            if (remain != 0)
                return false;
        }
        return true;
    }

    bool allSlotsNonNegative() const
    {
        for (const auto& p : compatCount)
            if (p.second < 0)
                return false;
        return true;
    }
};

struct LibraryArtifact {
    // clang-format off
    enum class TreasureClass { Treasure, Minor, Major, Relic, Unique, Complex, BattleMachine, Scroll, Special };
    enum class OrderCategory  { Special, Stats, Skills, Magic, Income , Misc, Complex, Scrolls  };
    enum class SpecialEffect { None, NeutralDiplomacy, FactionsAlliance, AlwaysFly, AlwaysWaterWalk, ResurrectFangarms,
                               ExtendedNecromancy, DragonsBuffs, DisableSurrender, NoDamageWhirl, NoTerrainPenalty, BreakImmunities, PermanentDeath };
    enum class Tag { Invalid, Stats, Control };
    // clang-format on

    struct Presentation {
        OrderCategory orderCategory = OrderCategory::Special;
        int           orderGroup    = 0;
        int           order         = 0;
        std::string   iconStash;
        std::string   iconBonus;

        TranslationMap name;
        TranslationMap descr;
    };

    std::string              id;
    HeroPrimaryParams        statBonus;
    std::vector<std::string> calc;
    ArtifactSlotType         slot;
    TreasureClass            treasureClass = TreasureClass::Special;
    SpecialEffect            special       = SpecialEffect::None;
    std::string              untranslatedName;
    int                      legacyId = -1;

    std::vector<Tag> tags;

    int cost  = -1;
    int value = -1;
    int guard = -1;

    LibrarySpellConstPtr            scrollSpell = nullptr;
    SpellFilter                     provideSpells;
    SpellFilter                     protectSpells;
    SpellFilter                     forbidSpells;
    SpellCastParamsList             spellCasts;
    std::vector<RangeAttackPenalty> disabledPenalties;
    std::set<LibrarySpellConstPtr>  provideSpellsCache;

    std::vector<LibraryArtifactConstPtr> parts;
    LibraryUnitConstPtr                  battleMachineUnit = nullptr;

    ArtifactSlotRequirement slotReq;
    LibraryArtifactConstPtr partOfSet = nullptr;

    bool isWaterContent     = false;
    bool isEnabledByDefault = true;

    ObjectDefMappings objectDefs;
    Presentation      presentationParams;

    constexpr auto sortOrdering() const noexcept { return std::tie(presentationParams.orderCategory, presentationParams.orderGroup, presentationParams.order, value); }
};

struct ArtifactFilter {
    std::vector<LibraryArtifactConstPtr>        onlyArtifacts;
    std::vector<LibraryArtifactConstPtr>        notArtifacts;
    std::vector<LibraryArtifact::TreasureClass> classes;

    std::vector<LibraryArtifact::Tag> tags;
    std::vector<LibraryArtifact::Tag> notTags;

    bool all = false;

    bool isDefault() const
    {
        return onlyArtifacts.empty()
               && notArtifacts.empty()
               && tags.empty()
               && notTags.empty()
               && classes.empty()
               && !all;
    }

    bool contains(LibraryArtifactConstPtr art) const
    {
        if (!art)
            return false;

        if (isDefault())
            return false;

        if (containsAll())
            return true;

        bool result = true;
        if (!onlyArtifacts.empty()) {
            result = result && std::find(onlyArtifacts.cbegin(), onlyArtifacts.cend(), art) != onlyArtifacts.cend();
        }
        if (!notArtifacts.empty()) {
            result = result && std::find(notArtifacts.cbegin(), notArtifacts.cend(), art) == notArtifacts.cend();
        }
        if (!classes.empty()) {
            result = result && std::find(classes.cbegin(), classes.cend(), art->treasureClass) != classes.cend();
        }
        if (!tags.empty()) {
            bool tagsMatch = false;
            for (auto tag : tags) {
                if (std::find(art->tags.cbegin(), art->tags.cend(), tag) != art->tags.cend()) {
                    tagsMatch = true;
                    break;
                }
            }
            result = result && tagsMatch;
        }
        if (!notTags.empty()) {
            bool tagsMatch = false;
            for (auto tag : notTags) {
                if (std::find(art->tags.cbegin(), art->tags.cend(), tag) != art->tags.cend()) {
                    tagsMatch = true;
                    break;
                }
            }
            result = result && !tagsMatch;
        }

        return result;
    }

    bool containsAll() const
    {
        if (isDefault())
            return false;

        if (all)
            return true;

        if (!classes.empty()) {
            auto classesTmp = classes;
            std::sort(classesTmp.begin(), classesTmp.end());
            if (classesTmp == std::vector{ LibraryArtifact::TreasureClass::Treasure, LibraryArtifact::TreasureClass::Minor, LibraryArtifact::TreasureClass::Major, LibraryArtifact::TreasureClass::Relic })
                return true;
        }

        return false;
    }

    bool contains(LibraryArtifact::TreasureClass treasureClass) const
    {
        if (containsAll())
            return true;

        return std::find(classes.cbegin(), classes.cend(), treasureClass) != classes.cend();
    }

    std::vector<LibraryArtifactConstPtr> filterPossible(const std::vector<LibraryArtifact*>& allPossibleSpells) const
    {
        if (isDefault())
            return {};

        std::vector<LibraryArtifactConstPtr> populatedFilter;
        populatedFilter.reserve(allPossibleSpells.size());
        for (auto* spell : allPossibleSpells) {
            if (contains(spell))
                populatedFilter.push_back(spell);
        }
        return populatedFilter;
    }
    std::vector<LibraryArtifactConstPtr> filterPossible(const std::vector<LibraryArtifactConstPtr>& allPossibleSpells) const
    {
        if (isDefault())
            return {};

        std::vector<LibraryArtifactConstPtr> populatedFilter;
        populatedFilter.reserve(allPossibleSpells.size());
        for (auto* spell : allPossibleSpells) {
            if (contains(spell))
                populatedFilter.push_back(spell);
        }
        return populatedFilter;
    }

    void makeUnion(const ArtifactFilter& another)
    {
        if (another.isDefault())
            return;

        all = all || another.all;

        for (auto* art : another.onlyArtifacts) {
            if (std::find(onlyArtifacts.cbegin(), onlyArtifacts.cend(), art) == onlyArtifacts.cend())
                onlyArtifacts.push_back(art);
        }
        auto tmp = notArtifacts;
        for (auto* art : tmp) {
            if (another.contains(art))
                notArtifacts.erase(std::find(notArtifacts.cbegin(), notArtifacts.cend(), art));
        }
        for (auto tc : another.classes) {
            if (std::find(classes.cbegin(), classes.cend(), tc) == classes.cend())
                classes.push_back(tc);
        }
        std::sort(classes.begin(), classes.end());
        for (auto tag : another.tags) {
            if (std::find(tags.cbegin(), tags.cend(), tag) == tags.cend())
                tags.push_back(tag);
        }
        for (auto tag : another.notTags) {
            if (std::find(notTags.cbegin(), notTags.cend(), tag) == notTags.cend())
                notTags.push_back(tag);
        }

        auto onlyArtifactsCopy = onlyArtifacts;
        onlyArtifacts.clear();
        if (isDefault()) {
            onlyArtifacts = onlyArtifactsCopy;
            return;
        }
        for (auto* art : onlyArtifactsCopy) {
            if (!contains(art))
                onlyArtifacts.push_back(art);
        }
    }

    bool operator==(const ArtifactFilter&) const noexcept = default;

    std::string toPrintable() const
    {
        std::string result;
        if (!classes.empty()) {
            result += "TC=";
            for (auto tag : classes)
                result += std::to_string((int) tag) + ", ";
        }
        if (!tags.empty()) {
            result += "; tags=";
            for (auto tag : tags)
                result += std::to_string((int) tag) + ", ";
        }
        if (!notTags.empty()) {
            result += "; notTags=";
            for (auto tag : notTags)
                result += std::to_string((int) tag) + ", ";
        }
        return result;
    }
};

using ArtifactReward = std::vector<ArtifactFilter>;

}
