/*
 * Copyright (C) 2020 Smirnov Vladimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "CursorLibrary.hpp"

namespace FreeHeroes::Gui {

CursorLibrary::CursorLibrary(const IGraphicsLibrary* graphicsLibrary)
{
    // crdeflt  cradvntr
    auto sprite    = graphicsLibrary->getObjectAnimation("wrcombat")->get();
    auto spellCast = graphicsLibrary->getObjectAnimation("crspell")->get();
    if (!sprite || !spellCast)
        return;
    auto frames                           = sprite->getFramesForGroup(0);
    auto spellCastFrames                  = spellCast->getFramesForGroup(0);
    otherCursors[Type::Stop]              = QCursor(frames->m_frames[0].m_frame, 13, 13);
    otherCursors[Type::Walk]              = QCursor(frames->m_frames[1].m_frame, 12, 25);
    otherCursors[Type::Fly]               = QCursor(frames->m_frames[2].m_frame, 12, 25);
    otherCursors[Type::RangeAttack]       = QCursor(frames->m_frames[3].m_frame, 22, 3);
    otherCursors[Type::HeroView]          = QCursor(frames->m_frames[4].m_frame, 14, 12);
    otherCursors[Type::Question]          = QCursor(frames->m_frames[5].m_frame, 9, 12);
    otherCursors[Type::PlainArrow]        = QCursor(frames->m_frames[6].m_frame, 0, 0);
    otherCursors[Type::RangeAttackBroken] = QCursor(frames->m_frames[15].m_frame, 27, 15);

    for (auto& f : spellCastFrames->m_frames) {
        cast << QCursor(f.m_frame, f.m_frame.width() / 2, f.m_frame.height() - 1);
    }

    QPixmap sword = frames->m_frames[13].m_frame;
    for (auto attackDirection : {
             BattleDirection::TR,
             BattleDirection::R,
             BattleDirection::BR,
             BattleDirection::BL,
             BattleDirection::L,
             BattleDirection::TL,
             BattleDirection::Top,
             BattleDirection::Bottom,
         }) {
        int hotX = -1, hotY = -1, degree = 0;
        // clang-format off
        switch (attackDirection) {
        case BattleDirection::TR    : hotX = 22; hotY = 4 ; degree = 30 ; break;
        case BattleDirection::R     : hotX = 31; hotY = 6 ; degree = 90 ; break;
        case BattleDirection::BR    : hotX = 22; hotY = 28; degree = 150; break;
        case BattleDirection::BL    : hotX = 6 ; hotY = 31; degree = 210; break;
        case BattleDirection::L     : hotX = 0 ; hotY = 6 ; degree = 270; break;
        case BattleDirection::TL    : hotX = 6 ; hotY = 4 ; degree = 330; break;
        case BattleDirection::Top   : hotX = 6 ; hotY = 0 ; degree = 0  ; break;
        case BattleDirection::Bottom: hotX = 6 ; hotY = 31; degree = 180; break;
        default:
            break;
        }
        // clang-format on
        QCursor cur(sword.transformed(QTransform().rotate(degree)), hotX, hotY);
        attackCursors[attackDirection] = cur;
    }
}

}
