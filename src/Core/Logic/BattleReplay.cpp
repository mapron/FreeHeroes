/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "BattleReplay.hpp"

#include <stdexcept>

namespace FreeHeroes::Core {
BattleReplayPlayer::BattleReplayPlayer(IBattleControl&   sourceControl,
                                       BattleReplayData& data)
    : m_sourceControl(sourceControl)
    , m_data(data)
{
}

BattleReplayPlayer::~BattleReplayPlayer() = default;

void BattleReplayPlayer::rewindToStart()
{
    m_pos = 0;
}

size_t BattleReplayPlayer::getSize() const
{
    return m_data.m_records.size();
}

size_t BattleReplayPlayer::getPos() const
{
    return m_pos;
}

bool BattleReplayPlayer::executeCurrent()
{
    if (m_pos >= m_data.m_records.size())
        return false;

    if (!handleEvent(m_data.m_records[m_pos])) {
        throw std::runtime_error("Battle manager can not repeat this from pos:" + std::to_string(m_pos));
        return false;
    }

    m_pos++;
    return true;
}

bool BattleReplayPlayer::handleEvent(const BattleReplayData::EventRecord& rec)
{
    // clang-format off
    switch (rec.type) {
        case BattleReplayData::EventRecord::Type::Guard      : return m_sourceControl.doGuard();
        case BattleReplayData::EventRecord::Type::Wait       : return m_sourceControl.doWait();
        case BattleReplayData::EventRecord::Type::MoveAttack : return m_sourceControl.doMoveAttack(rec.moveParams, rec.attackParams);
        case BattleReplayData::EventRecord::Type::Cast       : return m_sourceControl.doCast(rec.castParams);
        default:
            break;
    }
    // clang-format on
    throw std::runtime_error("Unknown event type");
    return false;
}

BattleReplayRecorder::BattleReplayRecorder(IBattleControl& sourceControl, BattleReplayData& data)
    : m_sourceControl(sourceControl)
    , m_data(data)
{
}

BattleReplayRecorder::~BattleReplayRecorder() = default;

bool BattleReplayRecorder::doGuard()
{
    auto res = m_sourceControl.doGuard();
    if (res)
        m_data.m_records.push_back({ BattleReplayData::EventRecord::Type::Guard, {}, {}, {} });
    return res;
}

bool BattleReplayRecorder::doWait()
{
    auto res = m_sourceControl.doWait();
    if (res)
        m_data.m_records.push_back({ BattleReplayData::EventRecord::Type::Wait, {}, {}, {} });
    return res;
}

bool BattleReplayRecorder::doMoveAttack(BattlePlanMoveParams moveParams, BattlePlanAttackParams attackParams)
{
    auto res = m_sourceControl.doMoveAttack(moveParams, attackParams);
    if (res)
        m_data.m_records.push_back({ BattleReplayData::EventRecord::Type::MoveAttack, moveParams, attackParams, {} });
    return res;
}

bool BattleReplayRecorder::doCast(BattlePlanCastParams planParams)
{
    auto res = m_sourceControl.doCast(planParams);
    if (res)
        m_data.m_records.push_back({ BattleReplayData::EventRecord::Type::Cast, {}, {}, planParams });
    return res;
}

}
