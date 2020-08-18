/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include "LibraryFwd.hpp"
#include "LibrarySpell.hpp"

#include <string>
#include <vector>
#include <set>

#include <cassert>

namespace FreeHeroes::Core {

enum class ArtifactSlotType { Sword, Shield, Helm, Torso,
                              Ring, Neck, Boots, Cape, Misc,
                              BmShoot, BmAmmo, BmTent,
                              Ring1, Misc1, Misc2, Misc3, Misc4,
                              Invalid, BagOnly };

struct ArtifactWearingSet {
    std::set<ArtifactSlotType> wearingSlots;

    static ArtifactSlotType getCompatibleSlotType(ArtifactSlotType wearingSlot) {
        ArtifactSlotType compatSlot = wearingSlot;
        if (compatSlot == ArtifactSlotType::Misc1 ||
            compatSlot == ArtifactSlotType::Misc2||
                compatSlot == ArtifactSlotType::Misc3||
                compatSlot == ArtifactSlotType::Misc4
                )
            compatSlot = ArtifactSlotType::Misc;
        if (compatSlot == ArtifactSlotType::Ring1)
            compatSlot = ArtifactSlotType::Ring;
        return compatSlot;
    }
    static std::vector<ArtifactSlotType> getCompatibleWearingSlots(ArtifactSlotType compatSlot) {
        if (compatSlot == ArtifactSlotType::Misc)
            return {ArtifactSlotType::Misc, ArtifactSlotType::Misc1, ArtifactSlotType::Misc2, ArtifactSlotType::Misc3, ArtifactSlotType::Misc4};
        if (compatSlot == ArtifactSlotType::Ring)
            return {ArtifactSlotType::Ring, ArtifactSlotType::Ring1};
        return {compatSlot};
    }
    ArtifactWearingSet operator + (const ArtifactWearingSet & rh) const {
        ArtifactWearingSet result = *this;
        for (auto & slot : rh.wearingSlots)
            result.wearingSlots.insert(slot);
        return result;
    }
    ArtifactWearingSet operator - (const ArtifactWearingSet & rh) const {
        ArtifactWearingSet result;
        for (auto & slot : wearingSlots)
            if (!rh.wearingSlots.contains(slot))
                result.wearingSlots.insert(slot);
        return result;
    }
    static const ArtifactWearingSet & defaultFreeSlots() {
        using A = ArtifactSlotType;
        static const ArtifactWearingSet heroSlots{{A::Sword, A::Shield, A::Helm, A::Torso,
                        A::Ring, A::Neck, A::Boots, A::Cape, A::Misc,
                        A::Ring1, A::Misc1, A::Misc2, A::Misc3, A::Misc4}};
        return heroSlots;
    }
};

struct ArtifactSlotRequirement {
    std::map<ArtifactSlotType, int> compatCount;
    bool canFitInto(const ArtifactSlotRequirement & containerFree) const {
        for (const auto & p : compatCount) {
            ArtifactSlotType type = p.first;
            if (!containerFree.compatCount.contains(type))
                return false;
            const int thisCount = p.second;
            const int containerCount = containerFree.compatCount.at(type);
            if (thisCount > containerCount)
                return false;
        }
        return true;
    }
    static const ArtifactSlotRequirement & defaultFreeSlots() {
         using A = ArtifactSlotType;
        static const ArtifactSlotRequirement heroSlots{
            {{A::Sword,1}, {A::Shield,1}, {A::Helm,1}, {A::Torso,1},
             {A::Ring,2}, {A::Neck,1}, {A::Boots,1}, {A::Cape,1}, {A::Misc,5}}
        };
        return heroSlots;
    }
    void subtract(const ArtifactSlotRequirement & req) {
        for (const auto & p : req.compatCount)
            compatCount[p.first] -= p.second;
    }
    void subtract(ArtifactSlotType compatSlot) {
        compatCount[compatSlot]--;
    }
    void add(const ArtifactSlotRequirement & req) {
        for (const auto & p : req.compatCount)
            compatCount[p.first] += p.second;
    }
    void add(ArtifactSlotType compatSlot) {
        compatCount[compatSlot]++;
    }
    ArtifactSlotRequirement operator - (const ArtifactSlotRequirement & rh) const {
        ArtifactSlotRequirement result = *this;
        result.subtract(rh);
        return result;
    }
    ArtifactSlotRequirement operator - (const ArtifactSlotType & rh) const {
        ArtifactSlotRequirement result = *this;
        result.subtract(rh);
        return result;
    }
    ArtifactSlotRequirement operator + (const ArtifactSlotRequirement & rh) const {
        ArtifactSlotRequirement result = *this;
        result.add(rh);
        return result;
    }
    ArtifactSlotRequirement operator + (const ArtifactSlotType & rh) const {
        ArtifactSlotRequirement result = *this;
        result.add(rh);
        return result;
    }

    bool getWearingTypes(ArtifactWearingSet & result, const ArtifactWearingSet & bannedSlots) const {
        for (const auto & p : compatCount) {
            int count = p.second;
            assert(count >=0);
            if (count == 0)
                continue;

            auto wearingList = ArtifactWearingSet::getCompatibleWearingSlots(p.first);
            if (count > (int)wearingList.size())
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
            if (remain !=0)
                return false;
        }
        return true;
    }

    bool allSlotsNonNegative() const {
        for (const auto & p : compatCount)
            if (p.second < 0)
                return false;
        return true;
    }
};


struct LibraryArtifact {
    enum class TreasureClass { Treasure, Minor, Major, Relic, Unique, Complex, BattleMachine, Scroll, Special };
    enum class OrderCategory  { Special, Stats, Skills, Magic, Income , Misc, Complex, Scrolls  };
    struct Presentation {
        OrderCategory orderCategory = OrderCategory::Special;
        int orderGroup = 0;
        int order = 0;
        std::string  iconStash;
        std::string  iconBonus;
    };

    std::string id;
    std::vector<std::string> calc;
    ArtifactSlotType slot;
    TreasureClass treasureClass = TreasureClass::Special;
    std::string  untranslatedName;

    int value = 0;
    LibrarySpellConstPtr  scrollSpell = nullptr;
    SpellFilter provideSpells;
    SpellFilter protectSpells;
    SpellFilter forbidSpells;
    SpellCastParamsList spellCasts;
    std::vector<RangeAttackPenalty> disabledPenalties;
    std::set<LibrarySpellConstPtr> provideSpellsCache;

    std::vector<LibraryArtifactConstPtr> parts;
    LibraryUnitConstPtr battleMachineUnit = nullptr;

    ArtifactSlotRequirement slotReq;
    LibraryArtifactConstPtr partOfSet = nullptr;

    Presentation presentationParams;

    constexpr auto sortOrdering() const noexcept { return std::tie(presentationParams.orderCategory, presentationParams.orderGroup, presentationParams.order, value);}
};

struct ArtifactRewardAmount {
    struct SingleReward {
        LibraryArtifact::TreasureClass treasureClass = LibraryArtifact::TreasureClass::Special;
        int count = 0;
    };

    std::vector<SingleReward> artifacts;

    size_t totalAmount() const noexcept {
        size_t res = 0;
        for (const auto & sr : artifacts)
            res += sr.count;
        return res;
    }
};

}
