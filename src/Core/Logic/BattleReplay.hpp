/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "CoreLogicExport.hpp"

#include "IBattleControl.hpp"
#include "IReplayHandle.hpp"

#include "AdventureArmy.hpp"
#include "LibraryFwd.hpp"
#include "BattleField.hpp"
#include "FsUtils.hpp"

#include <memory>
#include <deque>

namespace FreeHeroes::Core {

struct AdventureState
{
    AdventureArmy m_att;
    AdventureArmy m_def;
    LibraryTerrainConstPtr m_terrain = nullptr;
    uint64_t m_seed = 0;
    BattleFieldPreset m_field;
    LibraryMapObjectConstPtr m_mapObject = nullptr;
    int m_mapObjectVariant = 0;
};

struct BattleReplayData
{
    struct EventRecord {
        enum class Type { Guard, Wait, MoveAttack, Cast, Unknown };
        Type type = Type::Unknown;
        BattlePlanMoveParams moveParams;
        BattlePlanAttackParams attackParams;
        BattlePlanCastParams castParams;
    };
    std::deque<EventRecord> m_records;
};


class CORELOGIC_EXPORT BattleReplayPlayer : public IReplayHandle
{
public:
    BattleReplayPlayer(IBattleControl & sourceControl,
                       BattleReplayData & data);
    ~BattleReplayPlayer();

    void rewindToStart() override;
    size_t getSize() const override;
    size_t getPos() const override;
    bool executeCurrent() override;

    static void registerRTTR();
private:
    bool handleEvent(const BattleReplayData::EventRecord & rec);

private:
    IBattleControl& m_sourceControl;
    const BattleReplayData & m_data;
    size_t m_pos = 0;
};

class CORELOGIC_EXPORT BattleReplayRecorder : public IBattleControl
{
public:
    BattleReplayRecorder(IBattleControl & sourceControl, BattleReplayData & data);
    ~BattleReplayRecorder();

    bool doGuard() override;
    bool doWait() override;
    bool doMoveAttack(BattlePlanMoveParams moveParams, BattlePlanAttackParams attackParams) override;
    bool doCast(BattlePlanCastParams planParams) override;

private:
    struct EventRecord;
    void handleEvent(const EventRecord & rec);
private:
    IBattleControl& m_sourceControl;

    BattleReplayData & m_data;
};

}
