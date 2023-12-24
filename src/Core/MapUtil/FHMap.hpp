/*
 * Copyright (C) 2022 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <compare>
#include <set>
#include <optional>

#include "MernelPlatform/PropertyTree.hpp"
#include "GameConstants.hpp"

#include "AdventureArmy.hpp"
#include "Reward.hpp"

#include "LibrarySecondarySkill.hpp"

#include "FHTileMap.hpp"
#include "FHMapObject.hpp"
#include "FHTemplate.hpp"

#include "MapUtilExport.hpp"

namespace FreeHeroes {

namespace Core {
class IGameDatabase;
class IRandomGenerator;
}

struct FHPlayer {
    bool m_humanPossible{ false };
    bool m_aiPossible{ false };

    bool m_generateHeroAtMainTown{ false };

    std::vector<Core::LibraryFactionConstPtr> m_startingFactions;

    int m_team{ -1 };

    bool operator==(const FHPlayer&) const noexcept = default;
};

struct FHHeroData {
    bool m_hasExp        = false;
    bool m_hasSecSkills  = false;
    bool m_hasPrimSkills = false;
    bool m_hasCustomBio  = false;
    bool m_hasSpells     = false;

    Core::AdventureArmy m_army;

    bool operator==(const FHHeroData&) const noexcept = default;
};

struct FHGuard {
    bool                 m_hasGuards = false;
    Core::AdventureSquad m_creatures;

    bool operator==(const FHGuard&) const noexcept = default;
};

struct FHMessageWithBattle {
    bool        m_hasMessage = false;
    std::string m_message;
    FHGuard     m_guards;

    bool operator==(const FHMessageWithBattle&) const noexcept = default;
};

struct FHHero : public FHPlayerControlledObject {
    bool       m_isMain{ false };
    FHHeroData m_data;

    uint32_t m_questIdentifier = 0;

    bool operator==(const FHHero&) const noexcept = default;
};

struct FHDwelling : public FHPlayerControlledObject {
    Core::LibraryDwellingConstPtr m_id = nullptr;

    bool operator==(const FHDwelling&) const noexcept = default;
};

struct FHMine : public FHPlayerControlledObject {
    Core::LibraryResourceConstPtr m_id = nullptr;

    bool operator==(const FHMine&) const noexcept = default;
};

struct FHAbandonedMine : public FHCommonObject {
    std::vector<Core::LibraryResourceConstPtr> m_resources;

    bool operator==(const FHAbandonedMine&) const noexcept = default;
};

struct FHResource : public FHCommonObject {
    uint32_t                      m_amount = 0;
    Core::LibraryResourceConstPtr m_id     = nullptr;

    bool operator==(const FHResource&) const noexcept = default;
};

struct FHRandomResource : public FHCommonObject {
    uint32_t m_amount = 0;

    bool operator==(const FHRandomResource&) const noexcept = default;
};

struct FHArtifact : public FHCommonObject {
    Core::LibraryArtifactConstPtr m_id = nullptr;

    bool operator==(const FHArtifact&) const noexcept = default;
};

struct FHRandomArtifact : public FHCommonObject {
    enum class Type
    {
        Invalid,
        Any,
        Treasure,
        Minor,
        Major,
        Relic,
    };

    Type m_type = Type::Invalid;

    bool operator==(const FHRandomArtifact&) const noexcept = default;
};

struct FHPandora : public FHCommonObject {
    Core::Reward m_reward;
    std::string  m_key;
    bool         m_openPandora = false; // turn creature to joinable guard

    bool operator==(const FHPandora&) const noexcept = default;
};

struct FHMonster : public FHCommonObject {
    Core::LibraryUnitConstPtr m_id    = nullptr;
    uint32_t                  m_count = 0;

    int m_aggressionMin = 1;
    int m_aggressionMax = 10;

    bool m_joinOnlyForMoney = false;
    int  m_joinPercent      = 100;
    bool m_neverFlees       = false;
    bool m_notGrowingTeam   = false;

    uint32_t m_questIdentifier = 0;
    int64_t  m_guardValue      = 0;

    Core::Reward m_reward;

    enum class UpgradedStack
    {
        Invalid,
        Random,
        Yes,
        No
    };

    UpgradedStack m_upgradedStack = UpgradedStack::Random;

    bool operator==(const FHMonster&) const noexcept = default;
};

struct FHBank : public FHCommonObject {
    Core::LibraryMapBankConstPtr m_id = nullptr;
    enum class UpgradedStack
    {
        Invalid,
        Random,
        Yes,
        No
    };

    UpgradedStack m_upgradedStack = UpgradedStack::Random;
    int           m_guardsVariant = -1; // -1 = full random

    std::vector<Core::LibraryArtifactConstPtr> m_artifacts; // empty = full random

    bool operator==(const FHBank&) const noexcept = default;
};

struct FHObstacle : public FHCommonObject {
    Core::LibraryMapObstacleConstPtr m_id = nullptr;

    bool operator==(const FHObstacle&) const noexcept = default;
};
struct FHVisitable : public FHCommonVisitable {
    bool operator==(const FHVisitable&) const noexcept = default;
};
struct FHVisitableControlled : public FHCommonVisitable {
    Core::LibraryPlayerConstPtr m_player = nullptr;

    bool operator==(const FHVisitableControlled&) const noexcept = default;
};
struct FHSign : public FHCommonVisitable {
    bool operator==(const FHSign&) const noexcept = default;

    std::string m_text;
};

struct FHShrine : public FHCommonVisitable {
    Core::LibrarySpellConstPtr m_spellId     = nullptr;
    int                        m_randomLevel = -1;

    bool operator==(const FHShrine&) const noexcept = default;
};

struct FHSkillHut : public FHCommonVisitable {
    std::vector<Core::LibrarySecondarySkillConstPtr> m_skillIds;

    bool operator==(const FHSkillHut&) const noexcept = default;
};
struct FHQuest {
    enum class Type
    {
        Invalid      = 0,
        GetHeroLevel = 1,
        GetPrimaryStat,
        KillHero,
        KillCreature,
        BringArtifacts,
        BringCreatures,
        BringResource,
        BeHero,
        BePlayer,
    };
    Type m_type = Type::Invalid;

    std::vector<Core::LibraryArtifactConstPtr> m_artifacts;
    std::vector<Core::UnitWithCount>           m_units;
    Core::ResourceAmount                       m_resources;
    Core::HeroPrimaryParams                    m_primary;
    int                                        m_level = 0;

    uint32_t m_targetQuestId = 0;

    bool operator==(const FHQuest&) const noexcept = default;
};

struct FHQuestHut : public FHCommonVisitable {
    struct FHQuestWithReward {
        Core::Reward m_reward;
        FHQuest      m_quest;

        bool operator==(const FHQuestWithReward&) const noexcept = default;
    };

    std::vector<FHQuestWithReward> m_questsOneTime;
    std::vector<FHQuestWithReward> m_questsRecurring;

    bool operator==(const FHQuestHut&) const noexcept = default;
};

struct FHQuestGuard : public FHCommonVisitable {
    FHQuest m_quest;

    bool operator==(const FHQuestGuard&) const noexcept = default;
};

struct FHScholar : public FHCommonVisitable {
    enum Type
    {
        Primary,
        Secondary,
        Spell,
        Random,
    };
    Type m_type = Type::Random;

    Core::HeroPrimaryParamType          m_primaryType = Core::HeroPrimaryParamType::Attack;
    Core::LibrarySecondarySkillConstPtr m_skillId     = nullptr;
    Core::LibrarySpellConstPtr          m_spellId     = nullptr;

    bool operator==(const FHScholar&) const noexcept = default;
};

struct FHLocalEvent : public FHCommonVisitable {
    FHMessageWithBattle m_message;
    Core::Reward        m_reward;

    std::vector<Core::LibraryPlayerConstPtr> m_players;

    bool m_computerActivate = false;
    bool m_removeAfterVisit = false;
    bool m_humanActivate    = true;

    bool operator==(const FHLocalEvent&) const noexcept = default;
};

struct FHGarison : public FHVisitableControlled {
    Core::AdventureSquad m_garison;
    bool                 m_removableUnits = true;

    bool operator==(const FHGarison&) const noexcept = default;
};
struct FHHeroPlaceholder : public FHPlayerControlledObject {
    uint8_t m_hero      = 0;
    uint8_t m_powerRank = 0;

    bool operator==(const FHHeroPlaceholder&) const noexcept = default;
};

struct FHGlobalMapEvent {
    std::string m_name;
    std::string m_message;

    Core::ResourceAmount                     m_resources;
    std::vector<Core::LibraryPlayerConstPtr> m_players;
    bool                                     m_humanAffected = true;

    bool     m_computerAffected = false;
    uint16_t m_firstOccurence   = 0;
    uint8_t  m_nextOccurence    = 0;

    bool operator==(const FHGlobalMapEvent&) const noexcept = default;
};

struct FHDebugTile {
    FHPos m_pos;

    int m_brushColor   = 0;   // 0 - transparent, -1 black, -2 white, 1..360 - hue, or 1..palette - paletted hue
    int m_brushAlpha   = 120; // 0..255
    int m_brushPalette = 0;   // 0 | 1..16 - amount colors in hue palette. if 0, then hue is final hue, if >0 then hue is made using equal hue spacing

    int m_penColor   = 0;   // see m_brushColor
    int m_penAlpha   = 180; // see m_brushAlpha
    int m_penPalette = 0;   // see m_brushPalette

    int m_textColor   = -2;  // see m_brushColor
    int m_textAlpha   = 255; // see m_brushAlpha
    int m_textPalette = 0;   // see m_brushPalette

    int m_shape       = 1; // 0 - none, 1 - circle, 2 - square
    int m_shapeRadius = 2; // 1..4

    std::string m_text;
};

struct MAPUTIL_EXPORT FHMap {
    using PlayersMap = std::map<Core::LibraryPlayerConstPtr, FHPlayer>;
    using DefMap     = std::map<Core::LibraryObjectDefConstPtr, Core::LibraryObjectDef>;

    Core::GameVersion m_version = Core::GameVersion::Invalid;
    uint64_t          m_seed{ 0 };

    FHTileMap m_tileMap;
    bool      m_tileMapUpdateRequired = true;

    std::string m_name;
    std::string m_descr;
    uint8_t     m_difficulty = 0;
    bool        m_isWaterMap = false;

    PlayersMap               m_players;
    std::vector<FHHero>      m_wanderingHeroes;
    std::vector<FHTown>      m_towns;
    std::vector<FHZone>      m_zones;
    std::vector<FHDebugTile> m_debugTiles;
    FHTemplate               m_template;

    struct Objects {
        std::vector<FHResource>            m_resources;
        std::vector<FHRandomResource>      m_resourcesRandom;
        std::vector<FHArtifact>            m_artifacts;
        std::vector<FHRandomArtifact>      m_artifactsRandom;
        std::vector<FHMonster>             m_monsters;
        std::vector<FHDwelling>            m_dwellings;
        std::vector<FHBank>                m_banks;
        std::vector<FHObstacle>            m_obstacles;
        std::vector<FHVisitable>           m_visitables;
        std::vector<FHVisitableControlled> m_controlledVisitables;
        std::vector<FHMine>                m_mines;
        std::vector<FHPandora>             m_pandoras;
        std::vector<FHShrine>              m_shrines;
        std::vector<FHSkillHut>            m_skillHuts;
        std::vector<FHScholar>             m_scholars;
        std::vector<FHQuestHut>            m_questHuts;
        std::vector<FHQuestGuard>          m_questGuards;
        std::vector<FHLocalEvent>          m_localEvents;
        std::vector<FHSign>                m_signs;
        std::vector<FHGarison>             m_garisons;
        std::vector<FHHeroPlaceholder>     m_heroPlaceholders;

        // only needed for RMG objects.
        template<typename T>
        inline std::vector<T>& container() noexcept
        {
            static_assert(sizeof(T) == 3);
            return T();
        }
        std::vector<const FHCommonObject*> getAllObjects() const
        {
            std::vector<const FHCommonObject*> result;
            result.reserve(m_resources.size()
                           + m_resourcesRandom.size()
                           + m_artifacts.size()
                           + m_artifactsRandom.size()
                           + m_monsters.size()
                           + m_dwellings.size()
                           + m_banks.size()
                           + m_obstacles.size()
                           + m_visitables.size()
                           + m_controlledVisitables.size()
                           + m_mines.size()
                           + m_pandoras.size()
                           + m_shrines.size()
                           + m_skillHuts.size()
                           + m_scholars.size()
                           + m_questHuts.size()
                           + m_questGuards.size()
                           + m_localEvents.size()
                           + m_signs.size()
                           + m_garisons.size()
                           + m_heroPlaceholders.size());
            for (auto& obj : m_resources)
                result.push_back(&obj);
            for (auto& obj : m_resourcesRandom)
                result.push_back(&obj);
            for (auto& obj : m_artifacts)
                result.push_back(&obj);
            for (auto& obj : m_artifactsRandom)
                result.push_back(&obj);
            for (auto& obj : m_monsters)
                result.push_back(&obj);
            for (auto& obj : m_dwellings)
                result.push_back(&obj);
            for (auto& obj : m_banks)
                result.push_back(&obj);
            for (auto& obj : m_obstacles)
                result.push_back(&obj);
            for (auto& obj : m_visitables)
                result.push_back(&obj);
            for (auto& obj : m_controlledVisitables)
                result.push_back(&obj);
            for (auto& obj : m_mines)
                result.push_back(&obj);
            for (auto& obj : m_pandoras)
                result.push_back(&obj);
            for (auto& obj : m_shrines)
                result.push_back(&obj);
            for (auto& obj : m_skillHuts)
                result.push_back(&obj);
            for (auto& obj : m_scholars)
                result.push_back(&obj);
            for (auto& obj : m_questHuts)
                result.push_back(&obj);
            for (auto& obj : m_questGuards)
                result.push_back(&obj);
            for (auto& obj : m_localEvents)
                result.push_back(&obj);
            for (auto& obj : m_signs)
                result.push_back(&obj);
            for (auto& obj : m_garisons)
                result.push_back(&obj);
            for (auto& obj : m_heroPlaceholders)
                result.push_back(&obj);
            return result;
        }
    } m_objects;

    struct Config {
        bool m_allowSpecialWeeks = true;
        bool m_hasRoundLimit     = false;
        int  m_roundLimit        = 100;
    } m_config;

    std::vector<FHRiver> m_rivers;
    std::vector<FHRoad>  m_roads;

    Core::LibraryTerrainConstPtr m_defaultTerrain = nullptr;

    template<class Ptr>
    struct DisableConfig {
        using Map = std::map<Ptr, bool>;
        Map m_data;

        bool isDisabled(bool isWater, Ptr obj) const
        {
            if (m_data.contains(obj))
                return m_data.at(obj);

            if (!isWater && obj->isWaterContent)
                return true;
            return !obj->isEnabledByDefault;
        }

        void setDisabled(bool isWater, Ptr obj, bool state)
        {
            if (!obj)
                return;

            if (state) {
                if (!obj->isEnabledByDefault) // if object is disabled by default = we don't add to the disabled, it's excess.
                    return;

                if (!isWater && obj->isWaterContent) // if object is for water map, and we have non-water map = we don't add to the disabled, it's excess.
                    return;

                m_data[obj] = true;
            } else {
                if (!isWater && !obj->isWaterContent && obj->isEnabledByDefault) // if object is for regular map, and we have non-water map = we don't add to the enabled, it's excess.
                    return;
                m_data[obj] = false;
            }
        }
    };

    using DisableConfigHeroes          = DisableConfig<Core::LibraryHeroConstPtr>;
    using DisableConfigArtifacts       = DisableConfig<Core::LibraryArtifactConstPtr>;
    using DisableConfigSpells          = DisableConfig<Core::LibrarySpellConstPtr>;
    using DisableConfigSecondarySkills = DisableConfig<Core::LibrarySecondarySkillConstPtr>;
    using DisableConfigBanks           = DisableConfig<Core::LibraryMapBankConstPtr>;

    DisableConfigHeroes          m_disabledHeroes;
    DisableConfigArtifacts       m_disabledArtifacts;
    DisableConfigSpells          m_disabledSpells;
    DisableConfigSecondarySkills m_disabledSkills;
    DisableConfigBanks           m_disabledBanks;

    std::vector<FHHeroData> m_customHeroes;

    std::vector<FHGlobalMapEvent> m_globalEvents;

    std::vector<Core::LibraryObjectDef> m_objectDefs;

    void toJson(Mernel::PropertyTree& data) const;
    void fromJson(Mernel::PropertyTree data, const Core::IGameDatabase* database);

    void applyRngUserSettings(const Mernel::PropertyTree& data, const Core::IGameDatabase* database);

    void initTiles(const Core::IGameDatabase* database);

    void rescaleToUserSize();
};

// clang-format off
template <> inline       std::vector<FHResource>                  &       FHMap::Objects::container()       noexcept { return m_resources;}
template <> inline       std::vector<FHRandomResource>            &       FHMap::Objects::container()       noexcept { return m_resourcesRandom;}
template <> inline       std::vector<FHArtifact>                  &       FHMap::Objects::container()       noexcept { return m_artifacts;}
template <> inline       std::vector<FHRandomArtifact>            &       FHMap::Objects::container()       noexcept { return m_artifactsRandom;}
template <> inline       std::vector<FHMonster>                   &       FHMap::Objects::container()       noexcept { return m_monsters;}
template <> inline       std::vector<FHDwelling>                  &       FHMap::Objects::container()       noexcept { return m_dwellings;}
template <> inline       std::vector<FHBank>                      &       FHMap::Objects::container()       noexcept { return m_banks;}
template <> inline       std::vector<FHObstacle>                  &       FHMap::Objects::container()       noexcept { return m_obstacles;}
template <> inline       std::vector<FHVisitable>                 &       FHMap::Objects::container()       noexcept { return m_visitables;}
template <> inline       std::vector<FHVisitableControlled>       &       FHMap::Objects::container()       noexcept { return m_controlledVisitables;}
template <> inline       std::vector<FHMine>                      &       FHMap::Objects::container()       noexcept { return m_mines;}
template <> inline       std::vector<FHPandora>                   &       FHMap::Objects::container()       noexcept { return m_pandoras;}
template <> inline       std::vector<FHShrine>                    &       FHMap::Objects::container()       noexcept { return m_shrines;}
template <> inline       std::vector<FHSkillHut>                  &       FHMap::Objects::container()       noexcept { return m_skillHuts;}
template <> inline       std::vector<FHScholar>                   &       FHMap::Objects::container()       noexcept { return m_scholars;}
template <> inline       std::vector<FHGarison>                   &       FHMap::Objects::container()       noexcept { return m_garisons;}
// clang-format on

}
