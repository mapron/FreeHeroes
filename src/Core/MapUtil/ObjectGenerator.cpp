/*
 * Copyright (C) 2023 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "IGameDatabase.hpp"
#include "IRandomGenerator.hpp"

#include "LibraryMapBank.hpp"

#include "ObjectGenerator.hpp"

namespace FreeHeroes {

struct ObjectBank : public ObjectGenerator::IObject {
    void                   setPos(FHPos pos) override { m_obj.m_pos = pos; }
    void                   place() const override { m_map->m_objects.m_banks.push_back(m_obj); }
    ObjectGenerator::Score getScore() const override { return m_score; }

    ObjectGenerator::Score m_score;
    FHBank                 m_obj;
    FHMap*                 m_map = nullptr;
};

struct ObjectFactoryBank : public ObjectGenerator::IObjectFactory {
    ObjectFactoryBank(FHMap& map, const Core::IGameDatabase* database, Core::IRandomGenerator* const rng)
        : m_rng(rng)
        , m_map(map)
    {
        for (auto* bank : database->mapBanks()->records()) {
            if (m_map.m_disabledBanks.isDisabled(m_map.m_isWaterMap, bank))
                continue;

            for (int i = 0, sz = (int) bank->variants.size(); i < sz; i++) {
                Record rec;
                rec.m_guardsVariant = i;
                rec.m_id            = bank;
                rec.m_score         = { { FHScoreSettings::Attr::Army, 10000 } };
                m_records.push_back(rec);
            }
        }

        for (auto* art : database->artifacts()->records()) {
            if (m_map.m_disabledArtifacts.isDisabled(m_map.m_isWaterMap, art))
                continue;
            m_artifacts.push_back(art);
        }
    }

    ObjectGenerator::IObjectPtr make() override
    {
        const auto& record = m_records[m_rng->gen(m_records.size() - 1)];

        const bool upgraded = record.m_id->upgradedStackIndex == -1 ? false : m_rng->genSmall(3) == 0;
        ObjectBank obj;
        obj.m_obj.m_id             = record.m_id;
        obj.m_obj.m_guardsVariant  = record.m_guardsVariant;
        obj.m_obj.m_upgradedStack  = upgraded ? FHBank::UpgradedStack::Yes : FHBank::UpgradedStack::No;
        obj.m_score                = record.m_score;
        obj.m_map                  = &m_map;
        const Core::Reward& reward = record.m_id->rewards[record.m_id->variants[record.m_guardsVariant].rewardIndex];

        {
            for (const auto& artFilter : reward.artifacts) {
                auto arts = artFilter.filterPossible(m_artifacts);
                if (arts.empty()) {
                    obj.m_obj.m_artifacts.push_back(nullptr);
                    continue;
                }
                obj.m_obj.m_artifacts.push_back(arts[m_rng->gen(arts.size() - 1)]);
            }
        }

        return std::make_shared<ObjectBank>(std::move(obj));
    }

    struct Record {
        Core::LibraryMapBankConstPtr m_id = nullptr;
        ObjectGenerator::Score       m_score;
        int                          m_guardsVariant = 0;
    };

    std::vector<Record>                        m_records;
    std::vector<Core::LibraryArtifactConstPtr> m_artifacts;
    Core::IRandomGenerator* const              m_rng;
    FHMap&                                     m_map;
};

void ObjectGenerator::generate()
{
    {
        ObjectFactoryBank bankFactory(m_map, m_database, m_rng);
        if (!bankFactory.m_records.empty())
            m_objectFactories.push_back(std::make_shared<ObjectFactoryBank>(std::move(bankFactory)));
    }

    auto tryGen = [this]() -> bool {
        for (IObjectFactoryPtr& fac : m_objectFactories) {
            auto obj = fac->make();
            m_objects.push_back(obj);
        }
        return true;
    };

    for (int i = 0; i < 7; i++) {
        if (!tryGen())
            break;
    }
}

}
